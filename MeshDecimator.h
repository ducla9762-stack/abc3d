#ifndef _ABC3D_MESH_DECIMATOR_H
#define _ABC3D_MESH_DECIMATOR_H

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <queue>
#include <string>
#include <vector>

#include "mesh.h"

/// <summary>
/// Mesh decimation by quadric error edge collapse (Garland and Heckbert, "Surface Simplification Using Quadric
/// Error Metrics", SIGGRAPH 1997).
///
/// Every vertex carries a quadric Q, a symmetric 4x4 matrix such that v^T Q v (v = [x y z 1]) is the sum of
/// squared distances from v to the planes of the faces originally around it. Collapsing an edge (a, b) gives the
/// merged vertex Q_a + Q_b and places it where that sum is smallest. Edges are collapsed cheapest first from a
/// priority queue until the face budget is reached.
///
/// Choices on top of the paper:
///   - Plane quadrics are weighted by triangle area, so a big face counts for more than a sliver.
///   - Only real edges are collapse candidates (the paper's optional non-edge pairs are left out), so the
///     surface's topology never changes.
///   - Open boundaries get the paper's constraint planes: a plane through each boundary edge, perpendicular to
///     its face, with a large weight. Boundaries stay put unless that is the only way to reach the target.
///   - When the optimal point is ill-conditioned (flat or creased regions), the best point on the edge segment is
///     used instead of only testing the two ends and the midpoint.
///   - A collapse is rejected if it would break the manifold (link condition), turn a face over or make it
///     degenerate (fold-over check), or create a sliver worse than the face had before.
///
/// The half-edge mesh supports quads and n-gons but the quadric metric works on triangles, so the result is
/// always all triangles: quad edge loops are lost. The collapses run on a compact triangle list and the half-edge
/// mesh is rebuilt once at the end, which also drops every deleted face and stray vertex. Decimate() never leaves
/// the mesh half modified: on failure it is exactly as it was.
/// </summary>
class HalfEdgeMeshDecimator {
public:
	struct Options {
		/// Stop once the mesh has this many triangles or fewer.
		uint targetTriangles = 0;
		/// Stop when the cheapest collapse would cost more than this (squared distance, mesh local units).
		double maxError = std::numeric_limits<double>::infinity();
		/// Add constraint planes along open boundaries so their outline is kept.
		bool preserveBoundary = true;
		/// Weight of the boundary constraint planes relative to the face planes.
		double boundaryWeight = 1000.0;
		/// Fold-over check: a face around the collapse may rotate by at most acos(minNormalDot).
		double minNormalDot = 0.2;
		/// Sliver check: reject a collapse that makes a face with quality below this (0 = degenerate,
		/// 1 = equilateral) unless the face was already that bad.
		double minTriangleQuality = 0.05;
	};

	struct Result {
		uint trianglesBefore = 0;
		uint trianglesAfter = 0;
		uint verticesBefore = 0;
		uint verticesAfter = 0;
		uint collapses = 0;
		/// Largest quadric error of any collapse that was applied.
		double maxCollapseError = 0.0;
	};

	std::string error;

	explicit HalfEdgeMeshDecimator(HalfEdgeMesh& mesh) : mesh(mesh) {}

	/// <returns>How many triangles the mesh has once every face is triangulated (n-gon -> n - 2).</returns>
	static uint TriangleCount(const HalfEdgeMesh& mesh) {
		uint count = 0;
		for (uint f = 0; f < mesh.FaceCount(); ++f) {
			if (mesh.FaceIsNull(f)) { continue; }
			const uint n = mesh.FaceEdgeCount(f);
			if (n >= 3) { count += n - 2; }
		}
		return count;
	}

	/// <summary>
	/// Relative cost of decimating a mesh of `triangles` triangles to `ratio` (0 to 1), for time estimates. Seeding
	/// the queue costs about n log n, and the collapse loop adds about the same again for each triangle removed.
	/// Multiply by SecondsPerWorkUnit for a time.
	/// </summary>
	static double EstimateWork(uint triangles, float ratio) {
		const double keep = std::clamp(double(ratio), 0.0, 1.0);
		if (triangles < 2 || keep >= 1.0) { return 0.0; }
		const double n = double(triangles);
		return n * std::log2(n) * (0.2 + (1.0 - keep));
	}

	/// Seconds per unit of EstimateWork, from the Decimate.DISABLED_Timing test: 200 to 430 ns in Release from 8k to
	/// 520k triangles (large meshes are slower per unit as they fall out of cache), and about ten times that in
	/// Debug. This is only the starting point: the viewport replaces it with the rate it measures.
#ifdef _DEBUG // MSVC defines it for the debug runtime
	static constexpr double SecondsPerWorkUnit = 3000e-9;
#else
	static constexpr double SecondsPerWorkUnit = 300e-9;
#endif

	/// <summary>Decimates to `ratio` (0 to 1) of the triangulated face count.</summary>
	bool DecimateToRatio(float ratio, Result* result = nullptr) {
		Options options;
		const double clamped = std::clamp(double(ratio), 0.0, 1.0);
		options.targetTriangles = uint(std::lround(clamped * double(TriangleCount(mesh))));
		return Decimate(options, result);
	}

	/// <summary>Collapses edges until the face budget or the error limit is reached.</summary>
	/// <returns>
	/// True if the mesh was changed. False (with `error` set) if there was nothing to do, no edge could be
	/// collapsed, or the result could not be rebuilt; the mesh is then exactly as it was.
	/// </returns>
	bool Decimate(const Options& options, Result* result = nullptr) {
		error.clear();
		Result stats;
		if (!LoadTriangles()) { return false; }

		stats.trianglesBefore = liveTriangles;
		stats.verticesBefore = liveVertices;
		if (liveTriangles <= options.targetTriangles) {
			error = "decimate: the mesh already has " + std::to_string(liveTriangles) + " triangles";
			if (result) { *result = stats; }
			return false;
		}

		BuildQuadrics(options);

		// Seed the queue with every edge once
		std::priority_queue<Candidate, std::vector<Candidate>, std::greater<Candidate>> queue;
		std::vector<uint> neighbors;
		for (uint u = 0; u < uint(positions.size()); ++u) {
			if (!vertexAlive[u]) { continue; }
			Neighbors(u, neighbors);
			for (uint w : neighbors) {
				if (u < w) { queue.push(MakeCandidate(u, w)); }
			}
		}

		while (liveTriangles > options.targetTriangles && !queue.empty()) {
			const Candidate c = queue.top();
			queue.pop();

			// Lazy deletion: skip entries whose endpoints changed since they were queued
			if (!vertexAlive[c.u] || !vertexAlive[c.v] || stamp[c.u] != c.stampU || stamp[c.v] != c.stampV) { continue; }
			if (c.cost > options.maxError) { break; }
			if (!CanCollapse(c.u, c.v, c.target, options)) { continue; }

			Collapse(c.u, c.v, c.target);
			++stats.collapses;
			stats.maxCollapseError = std::max(stats.maxCollapseError, c.cost);

			Neighbors(c.u, neighbors);
			for (uint w : neighbors) { queue.push(MakeCandidate(c.u, w)); }
		}

		if (stats.collapses == 0) {
			error = "decimate: no edge could be collapsed without damaging the mesh";
			if (result) { *result = stats; }
			return false;
		}

		if (!Rebuild()) {
			if (result) { *result = stats; }
			return false;
		}

		stats.trianglesAfter = mesh.FaceCount();
		stats.verticesAfter = mesh.VertexCount();
		if (result) { *result = stats; }
		return true;
	}

private:
	using Vec3D = Vec3<double>;
	using Tri = std::array<uint, 3>;

	/// Symmetric 4x4 matrix, upper triangle stored row by row:
	/// [ a00 a01 a02 a03 ]
	/// [     a11 a12 a13 ]
	/// [         a22 a23 ]
	/// [             a33 ]
	struct Quadric {
		double a00 = 0, a01 = 0, a02 = 0, a03 = 0;
		double a11 = 0, a12 = 0, a13 = 0;
		double a22 = 0, a23 = 0;
		double a33 = 0;

		/// The squared distance to the plane n.p + d = 0 (n unit length), times `weight`.
		static Quadric FromPlane(const Vec3D& n, double d, double weight) {
			Quadric q;
			q.a00 = weight * n.x * n.x; q.a01 = weight * n.x * n.y; q.a02 = weight * n.x * n.z; q.a03 = weight * n.x * d;
			q.a11 = weight * n.y * n.y; q.a12 = weight * n.y * n.z; q.a13 = weight * n.y * d;
			q.a22 = weight * n.z * n.z; q.a23 = weight * n.z * d;
			q.a33 = weight * d * d;
			return q;
		}

		Quadric& operator+=(const Quadric& o) {
			a00 += o.a00; a01 += o.a01; a02 += o.a02; a03 += o.a03;
			a11 += o.a11; a12 += o.a12; a13 += o.a13;
			a22 += o.a22; a23 += o.a23;
			a33 += o.a33;
			return *this;
		}

		Quadric operator+(const Quadric& o) const { Quadric q = *this; q += o; return q; }

		/// v^T Q v with v = [p, 1]
		double Error(const Vec3D& p) const {
			const double x = p.x, y = p.y, z = p.z;
			return a00 * x * x + 2 * a01 * x * y + 2 * a02 * x * z + 2 * a03 * x
			     + a11 * y * y + 2 * a12 * y * z + 2 * a13 * y
			     + a22 * z * z + 2 * a23 * z
			     + a33;
		}

		/// The point minimizing Error: solves A p = -b where A is the upper-left 3x3 block and b the last column.
		/// <returns>False if A is too close to singular for the answer to mean anything.</returns>
		bool Minimize(Vec3D& p) const {
			// Cofactors of the symmetric 3x3 block
			const double c00 = a11 * a22 - a12 * a12;
			const double c01 = a02 * a12 - a01 * a22;
			const double c02 = a01 * a12 - a02 * a11;
			const double det = a00 * c00 + a01 * c01 + a02 * c02;

			// Compare against the matrix's own scale so the test works at any mesh size
			const double scale = (a00 + a11 + a22) / 3.0;
			if (!(scale > 0.0) || std::abs(det) <= 1e-6 * scale * scale * scale) { return false; }

			const double c11 = a00 * a22 - a02 * a02;
			const double c12 = a01 * a02 - a00 * a12;
			const double c22 = a00 * a11 - a01 * a01;
			const double inv = 1.0 / det;
			const double bx = -a03, by = -a13, bz = -a23;
			p.x = inv * (c00 * bx + c01 * by + c02 * bz);
			p.y = inv * (c01 * bx + c11 * by + c12 * bz);
			p.z = inv * (c02 * bx + c12 * by + c22 * bz);
			return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
		}

		/// The point on segment [p0, p1] minimizing Error. Error along the segment is a parabola in t.
		Vec3D MinimizeOnSegment(const Vec3D& p0, const Vec3D& p1) const {
			const Vec3D d = p1 - p0;
			// e(t) = Error(p0 + t d) = e0 + B t + C t^2, so e(1) = e0 + B + C and e(-1) = e0 - B + C
			const double e0 = Error(p0), ePlus = Error(p1), eMinus = Error(p0 - d);
			const double B = 0.5 * (ePlus - eMinus);
			const double C = 0.5 * (ePlus + eMinus) - e0;
			double t;
			if (C > 0.0) { t = std::clamp(-B / (2.0 * C), 0.0, 1.0); }
			else { t = e0 <= ePlus ? 0.0 : 1.0; } // flat or concave along the edge: best end wins
			return p0 + d * t;
		}
	};

	struct Candidate {
		double cost;
		uint u, v;
		uint stampU, stampV;
		Vec3D target;
		bool operator>(const Candidate& o) const { return cost > o.cost; }
	};

	HalfEdgeMesh& mesh;

	std::vector<Vec3D> positions;
	std::vector<Quadric> quadrics;
	std::vector<uint> stamp;            // bumped whenever a vertex moves or absorbs another
	std::vector<char> vertexAlive;
	std::vector<std::vector<uint>> vertexTris; // triangles around each vertex, may hold dead ones
	std::vector<Tri> tris;
	std::vector<char> triAlive;
	uint liveTriangles = 0;
	uint liveVertices = 0;

	/// Copies the mesh into a triangle list, triangulating quads and n-gons.
	bool LoadTriangles() {
		const std::vector<Vec3f>& P = mesh.vertPos;
		positions.resize(P.size());
		for (size_t i = 0; i < P.size(); ++i) { positions[i] = Vec3D(P[i].x, P[i].y, P[i].z); }

		tris.clear();
		std::vector<Vec3u> faceTris;
		for (uint f = 0; f < mesh.FaceCount(); ++f) {
			if (mesh.FaceIsNull(f)) { continue; }
			faceTris.clear();
			mesh.GetTriangulation(f, faceTris);
			for (const Vec3u& t : faceTris) {
				if (t.x >= P.size() || t.y >= P.size() || t.z >= P.size()) {
					error = "decimate: face " + std::to_string(f) + " references a missing vertex";
					return false;
				}
				tris.push_back({ t.x, t.y, t.z });
			}
		}
		if (tris.empty()) {
			error = "decimate: the mesh has no faces";
			return false;
		}

		triAlive.assign(tris.size(), 1);
		liveTriangles = uint(tris.size());
		vertexTris.assign(positions.size(), {});
		vertexAlive.assign(positions.size(), 0);
		stamp.assign(positions.size(), 0);
		for (uint t = 0; t < uint(tris.size()); ++t) {
			for (uint corner : tris[t]) {
				vertexTris[corner].push_back(t);
				vertexAlive[corner] = 1;
			}
		}
		liveVertices = uint(std::count(vertexAlive.begin(), vertexAlive.end(), char(1)));
		return true;
	}

	void BuildQuadrics(const Options& options) {
		quadrics.assign(positions.size(), Quadric());

		for (const Tri& t : tris) {
			const Vec3D& p0 = positions[t[0]];
			const Vec3D cross = (positions[t[1]] - p0).Cross(positions[t[2]] - p0);
			const double len = cross.Length();
			if (!(len > 0.0)) { continue; } // degenerate input face: no plane
			const Vec3D n = cross / len;
			const Quadric q = Quadric::FromPlane(n, -n.Dot(p0), 0.5 * len); // weight = area
			for (uint corner : t) { quadrics[corner] += q; }
		}

		if (!options.preserveBoundary) { return; }

		// A boundary edge belongs to one triangle. Its directed edge (a, b) then has no reverse (b, a).
		for (uint ti = 0; ti < uint(tris.size()); ++ti) {
			const Tri& t = tris[ti];
			for (int k = 0; k < 3; ++k) {
				const uint a = t[k], b = t[(k + 1) % 3];
				if (SharedTriangleCount(a, b) != 1) { continue; }

				const Vec3D& p0 = positions[t[0]];
				const Vec3D faceNormal = (positions[t[1]] - p0).Cross(positions[t[2]] - p0);
				const Vec3D edge = positions[b] - positions[a];
				Vec3D n = edge.Cross(faceNormal);
				const double len = n.Length();
				if (!(len > 0.0)) { continue; }
				n /= len;
				// Weight by squared edge length so it has the same units (area) as the face planes
				const Quadric q = Quadric::FromPlane(n, -n.Dot(positions[a]), options.boundaryWeight * edge.LengthSquared());
				quadrics[a] += q;
				quadrics[b] += q;
			}
		}
	}

	bool Contains(const Tri& t, uint v) const { return t[0] == v || t[1] == v || t[2] == v; }

	uint SharedTriangleCount(uint a, uint b) const {
		uint count = 0;
		for (uint t : vertexTris[a]) {
			if (triAlive[t] && Contains(tris[t], b)) { ++count; }
		}
		return count;
	}

	/// Sorted, unique vertices sharing a live triangle with `v`.
	void Neighbors(uint v, std::vector<uint>& out) const {
		out.clear();
		for (uint t : vertexTris[v]) {
			if (!triAlive[t]) { continue; }
			for (uint corner : tris[t]) {
				if (corner != v) { out.push_back(corner); }
			}
		}
		std::sort(out.begin(), out.end());
		out.erase(std::unique(out.begin(), out.end()), out.end());
	}

	/// True if any edge around `v` has only one triangle.
	bool IsBoundaryVertex(uint v) const {
		std::vector<uint> neighbors;
		Neighbors(v, neighbors);
		for (uint w : neighbors) {
			if (SharedTriangleCount(v, w) == 1) { return true; }
		}
		return false;
	}

	Candidate MakeCandidate(uint u, uint v) const {
		const Quadric q = quadrics[u] + quadrics[v];
		Vec3D target;
		const Vec3D& pu = positions[u];
		const Vec3D& pv = positions[v];
		bool solved = q.Minimize(target);
		if (solved) {
			// A barely invertible matrix can still put the point far away along its weak direction
			const double reach = 2.0 * (pv - pu).Length();
			if ((target - (pu + pv) * 0.5).Length() > reach) { solved = false; }
		}
		if (!solved) { target = q.MinimizeOnSegment(pu, pv); }

		// Rounding can push a true zero slightly negative
		const double cost = std::max(0.0, q.Error(target));
		return Candidate{ cost, u, v, stamp[u], stamp[v], target };
	}

	/// 1 for an equilateral triangle, 0 for a degenerate one.
	static double Quality(const Vec3D& a, const Vec3D& b, const Vec3D& c) {
		const double area2 = (b - a).Cross(c - a).Length(); // twice the area
		const double sumSq = (b - a).LengthSquared() + (c - b).LengthSquared() + (a - c).LengthSquared();
		if (!(sumSq > 0.0)) { return 0.0; }
		return 2.0 * std::sqrt(3.0) * area2 / sumSq;
	}

	bool CanCollapse(uint u, uint v, const Vec3D& target, const Options& options) const {
		// Triangles on the edge, and the vertex opposite the edge in each
		std::vector<uint> opposite;
		for (uint t : vertexTris[u]) {
			if (!triAlive[t] || !Contains(tris[t], v)) { continue; }
			for (uint corner : tris[t]) {
				if (corner != u && corner != v) { opposite.push_back(corner); }
			}
		}
		if (opposite.empty() || opposite.size() > 2) { return false; } // not an edge, or non-manifold
		std::sort(opposite.begin(), opposite.end());
		if (opposite.size() == 2 && opposite[0] == opposite[1]) { return false; }

		// Link condition: u and v may share no neighbor other than the vertices opposite the edge, or the collapse
		// would pinch the surface
		std::vector<uint> nu, nv, common;
		Neighbors(u, nu);
		Neighbors(v, nv);
		std::set_intersection(nu.begin(), nu.end(), nv.begin(), nv.end(), std::back_inserter(common));
		if (common != opposite) { return false; }

		// An interior edge joining two boundary vertices would pinch the boundary into a figure eight
		const bool boundaryEdge = opposite.size() == 1;
		if (!boundaryEdge && IsBoundaryVertex(u) && IsBoundaryVertex(v)) { return false; }

		// A vertex opposite the edge loses one neighbor. Keep at least three around an interior vertex (two would
		// leave a pair of back-to-back faces, as when a tetrahedron is collapsed) and two around a boundary one.
		for (uint w : opposite) {
			std::vector<uint> nw;
			Neighbors(w, nw);
			const uint minimum = IsBoundaryVertex(w) ? 3 : 4;
			if (nw.size() < minimum) { return false; }
		}

		// Fold-over and sliver checks on every triangle that survives the collapse
		for (uint end : { u, v }) {
			for (uint t : vertexTris[end]) {
				if (!triAlive[t]) { continue; }
				const Tri& tri = tris[t];
				if (Contains(tri, u) && Contains(tri, v)) { continue; } // removed by the collapse

				Vec3D before[3], after[3];
				for (int k = 0; k < 3; ++k) {
					before[k] = positions[tri[k]];
					after[k] = tri[k] == end ? target : before[k];
				}
				const Vec3D n0 = (before[1] - before[0]).Cross(before[2] - before[0]);
				const Vec3D n1 = (after[1] - after[0]).Cross(after[2] - after[0]);
				const double len0 = n0.Length(), len1 = n1.Length();
				if (!(len1 > 0.0)) { return false; }
				if (len0 > 0.0 && n0.Dot(n1) < options.minNormalDot * len0 * len1) { return false; }

				const double q1 = Quality(after[0], after[1], after[2]);
				if (q1 < 1e-6) { return false; } // too thin to survive the round trip to float
				if (q1 < options.minTriangleQuality && q1 < Quality(before[0], before[1], before[2])) { return false; }
			}
		}
		return true;
	}

	/// Merges v into u and moves u to `target`.
	void Collapse(uint u, uint v, const Vec3D& target) {
		for (uint t : vertexTris[v]) {
			if (!triAlive[t]) { continue; }
			Tri& tri = tris[t];
			if (Contains(tri, u)) {
				triAlive[t] = 0;
				--liveTriangles;
				continue;
			}
			for (uint& corner : tri) {
				if (corner == v) { corner = u; }
			}
			vertexTris[u].push_back(t);
		}

		// Drop dead triangles from u's list so it doesn't grow without bound
		std::vector<uint>& around = vertexTris[u];
		around.erase(std::remove_if(around.begin(), around.end(), [&](uint t) { return !triAlive[t]; }), around.end());

		vertexTris[v].clear();
		vertexAlive[v] = 0;
		--liveVertices;

		positions[u] = target;
		quadrics[u] += quadrics[v];
		++stamp[u];
	}

	/// Replaces the mesh with the surviving triangles, keeping its object transform.
	bool Rebuild() {
		std::vector<uint> remap(positions.size(), MAX_UINT);
		std::vector<Vec3f> vertices;
		std::vector<Vec4u> faces;
		faces.reserve(liveTriangles);

		for (uint t = 0; t < uint(tris.size()); ++t) {
			if (!triAlive[t]) { continue; }
			Vec4u face(0, 0, 0, MAX_UINT);
			for (int k = 0; k < 3; ++k) {
				const uint v = tris[t][k];
				if (remap[v] == MAX_UINT) {
					remap[v] = uint(vertices.size());
					vertices.push_back(Vec3f(float(positions[v].x), float(positions[v].y), float(positions[v].z)));
				}
				face[k] = remap[v];
			}
			faces.push_back(face);
		}

		HalfEdgeMesh rebuilt;
		rebuilt.vertPos = std::move(vertices);
		if (!rebuilt.build(faces, basicVertexCountFunc)) {
			error = "decimate: the decimated mesh could not be rebuilt";
			return false;
		}
		std::string err;
		if (!rebuilt.Validate(err)) {
			error = "decimate produced an invalid mesh: " + err;
			return false;
		}

		rebuilt.scale = mesh.scale;
		rebuilt.pos = mesh.pos;
		rebuilt.eulerRot = mesh.eulerRot;
		mesh = std::move(rebuilt);
		return true;
	}
};

#endif //_ABC3D_MESH_DECIMATOR_H
