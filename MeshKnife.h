#ifndef _ABC3D_MESH_KNIFE_H
#define _ABC3D_MESH_KNIFE_H

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mesh.h"

/// <summary>
/// The view the knife is cut through. Everything is in world space, the knife path is in
/// normalized device coordinates, so it does not care about window size or pixel ratio.
/// </summary>
struct KnifeCamera {
	Matrix4f viewProj;  // world -> clip, OpenGL conventions (the renderer's projection * view)
	Vec3f position;     // world-space eye position
};

/// <summary>
/// Triangles of every mesh in the scene, used to decide whether a point is hidden from the camera.
/// </summary>
class KnifeOccluders {
	struct Tri { Vec3f a, b, c; };
	struct Chunk {
		Vec3f lo, hi;
		std::vector<Tri> tris;
	};
	std::vector<Chunk> chunks;

public:
	/// <summary>Adds all faces of a mesh, in world space.</summary>
	void Add(const HalfEdgeMesh& mesh) {
		std::vector<Vec3f> world;
		mesh.GetWorldVertexPositions(world);
		if (world.empty()) { return; }

		Chunk chunk;
		chunk.lo = Vec3f(std::numeric_limits<float>::max());
		chunk.hi = Vec3f(std::numeric_limits<float>::lowest());
		for (const Vec3f& v : world) {
			for (int i = 0; i < 3; ++i) {
				chunk.lo[i] = std::min(chunk.lo[i], v[i]);
				chunk.hi[i] = std::max(chunk.hi[i], v[i]);
			}
		}

		std::vector<Vec3u> tris;
		for (uint f = 0; f < mesh.FaceCount(); ++f) {
			tris.clear();
			mesh.GetTriangulation(f, tris);
			for (const Vec3u& t : tris) {
				chunk.tris.push_back({ world[t.x], world[t.y], world[t.z] });
			}
		}

		chunks.push_back(std::move(chunk));
	}

	/// <returns>
	/// True if a triangle lies strictly between `from` and `to`. A small tolerance, relative to the distance,
	/// keeps the surface that `to` sits on from hiding it.
	/// </returns>
	bool Blocked(const Vec3f& from, const Vec3f& to) const {
		Vec3f dir = to - from;
		const float dist = dir.Length();
		if (dist < 1e-9f) { return false; }
		dir /= dist;
		const float maxT = dist - (dist * 1e-3f + 1e-4f);
		if (maxT <= 0.0f) { return false; }

		for (const Chunk& chunk : chunks) {
			if (!HitsBox(from, dir, maxT, chunk)) { continue; }
			for (const Tri& tri : chunk.tris) {
				if (HitsTriangle(from, dir, maxT, tri)) { return true; }
			}
		}
		return false;
	}

private:
	static bool HitsBox(const Vec3f& o, const Vec3f& d, float maxT, const Chunk& box) {
		float t0 = 0.0f, t1 = maxT;
		for (int i = 0; i < 3; ++i) {
			if (std::abs(d[i]) < 1e-12f) {
				if (o[i] < box.lo[i] || o[i] > box.hi[i]) { return false; }
				continue;
			}
			float a = (box.lo[i] - o[i]) / d[i];
			float b = (box.hi[i] - o[i]) / d[i];
			if (a > b) { std::swap(a, b); }
			t0 = std::max(t0, a);
			t1 = std::min(t1, b);
			if (t0 > t1) { return false; }
		}
		return true;
	}

	// Moller-Trumbore, double sided
	static bool HitsTriangle(const Vec3f& o, const Vec3f& d, float maxT, const Tri& tri) {
		const Vec3f e1 = tri.b - tri.a;
		const Vec3f e2 = tri.c - tri.a;
		const Vec3f p = d.Cross(e2);
		const float det = e1.Dot(p);
		if (std::abs(det) < 1e-12f) { return false; }
		const float inv = 1.0f / det;
		const Vec3f s = o - tri.a;
		const float u = s.Dot(p) * inv;
		if (u < -1e-6f || u > 1.0f + 1e-6f) { return false; }
		const Vec3f q = s.Cross(e1);
		const float v = d.Dot(q) * inv;
		if (v < -1e-6f || u + v > 1.0f + 1e-6f) { return false; }
		const float t = e2.Dot(q) * inv;
		return t > 0.0f && t < maxT;
	}
};

/// <summary>
/// The screen footprint of some meshes, used to tell when a knife segment passes over one of them.
/// It ignores what is in front of what, so it answers "is the line over this mesh", not "would it be cut".
/// </summary>
class KnifeScreenCover {
	struct Tri { Vec3f a, b, c; };
	struct Poly {
		Vec2f lo, hi;
		int count = 0;
		Vec2f pts[4]; // a triangle clipped to the near plane has at most 4 corners
	};
	std::vector<Tri> tris;
	std::vector<Poly> polys;

	static float Cross2(const Vec2f& a, const Vec2f& b) { return a.x * b.y - a.y * b.x; }

	static bool Inside(const Poly& poly, const Vec2f& p) {
		bool pos = false, neg = false;
		for (int i = 0; i < poly.count; ++i) {
			const float c = Cross2(poly.pts[(i + 1) % poly.count] - poly.pts[i], p - poly.pts[i]);
			pos |= c > 0.0f;
			neg |= c < 0.0f;
		}
		return !(pos && neg);
	}

	static bool SegmentsCross(const Vec2f& a, const Vec2f& b, const Vec2f& c, const Vec2f& d) {
		const Vec2f r = b - a, s = d - c;
		const float den = Cross2(r, s);
		if (std::abs(den) < 1e-20f) { return false; }
		const float t = Cross2(c - a, s) / den;
		const float u = Cross2(c - a, r) / den;
		return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
	}

public:
	/// <summary>Adds all faces of a mesh, in world space.</summary>
	void Add(const HalfEdgeMesh& mesh) {
		std::vector<Vec3f> world;
		mesh.GetWorldVertexPositions(world);
		std::vector<Vec3u> faceTris;
		for (uint f = 0; f < mesh.FaceCount(); ++f) {
			faceTris.clear();
			mesh.GetTriangulation(f, faceTris);
			for (const Vec3u& t : faceTris) { tris.push_back({ world[t.x], world[t.y], world[t.z] }); }
		}
	}

	bool Empty() const { return tris.empty(); }

	/// <summary>Projects the triangles to the screen. Call again whenever the camera moves.</summary>
	void Project(const Matrix4f& viewProj) {
		polys.clear();
		auto clip = [&](const Vec3f& p) {
			const Matrix4f& m = viewProj;
			return Vec4f(
				m(0, 0) * p.x + m(0, 1) * p.y + m(0, 2) * p.z + m(0, 3),
				m(1, 0) * p.x + m(1, 1) * p.y + m(1, 2) * p.z + m(1, 3),
				m(2, 0) * p.x + m(2, 1) * p.y + m(2, 2) * p.z + m(2, 3),
				m(3, 0) * p.x + m(3, 1) * p.y + m(3, 2) * p.z + m(3, 3));
		};

		for (const Tri& tri : tris) {
			const Vec4f c[3] = { clip(tri.a), clip(tri.b), clip(tri.c) };

			// Clip to the near plane (z + w > 0), so triangles that pass behind the camera still count
			Vec4f kept[4];
			int n = 0;
			for (int i = 0; i < 3; ++i) {
				const Vec4f& p = c[i];
				const Vec4f& q = c[(i + 1) % 3];
				const float dp = p.z + p.w, dq = q.z + q.w;
				if (dp > 0.0f) { kept[n++] = p; }
				if ((dp > 0.0f) != (dq > 0.0f)) { kept[n++] = p + (q - p) * (dp / (dp - dq)); }
			}
			if (n < 3) { continue; }

			Poly poly;
			poly.count = n;
			poly.lo = Vec2f(std::numeric_limits<float>::max());
			poly.hi = Vec2f(std::numeric_limits<float>::lowest());
			for (int i = 0; i < n; ++i) {
				const float w = std::max(kept[i].w, 1e-6f);
				poly.pts[i] = Vec2f(kept[i].x / w, kept[i].y / w);
				poly.lo.x = std::min(poly.lo.x, poly.pts[i].x); poly.lo.y = std::min(poly.lo.y, poly.pts[i].y);
				poly.hi.x = std::max(poly.hi.x, poly.pts[i].x); poly.hi.y = std::max(poly.hi.y, poly.pts[i].y);
			}
			polys.push_back(poly);
		}
	}

	/// <returns>True if the screen segment a-b (NDC) touches any projected triangle.</returns>
	bool Crosses(const Vec2f& a, const Vec2f& b) const {
		const Vec2f lo(std::min(a.x, b.x), std::min(a.y, b.y));
		const Vec2f hi(std::max(a.x, b.x), std::max(a.y, b.y));
		for (const Poly& poly : polys) {
			if (hi.x < poly.lo.x || lo.x > poly.hi.x || hi.y < poly.lo.y || lo.y > poly.hi.y) { continue; }
			if (Inside(poly, a) || Inside(poly, b)) { return true; }
			for (int i = 0; i < poly.count; ++i) {
				if (SegmentsCross(a, b, poly.pts[i], poly.pts[(i + 1) % poly.count])) { return true; }
			}
		}
		return false;
	}
};

/// <summary>
/// Cuts a half-edge mesh along a line drawn on the screen (Blender style knife tool).
///
/// The cut path is a polyline in normalized device coordinates. For each face, the path is clipped against the
/// face's projection on screen, and every piece inside becomes a chord. A chord can end
///   - on a vertex,
///   - on an edge (the edge is split there),
///   - or inside a face, where a path point (a click) lands on it, or where the path crosses itself.
/// Chords are then inserted using two Euler operators: make-edge-vertex (a spur toward a new interior vertex) and
/// make-edge-face (a chord that splits a face in two).
///
/// Nothing is drawn that does not touch the mesh. Unless cut through is on, chords whose ends or middle are hidden
/// behind geometry are discarded.
///
/// Cut() never leaves the mesh half-modified: the result is validated and the mesh is restored if it is not valid.
/// </summary>
class HalfEdgeMeshKnife {
	// Two points closer than this on screen are the same point. NDC is 2 wide, so 0.002 is about one pixel at 1000px.
	static constexpr float SNAP = 0.002f;
	// Edge parameters closer than this are the same point on the edge
	static constexpr float EDGE_EPS = 1e-4f;

	struct ClipPos { float x, y, z, w; };

	// A vertex of a face's screen polygon, after clipping to the near plane
	struct PolyVert {
		ClipPos clip;
		Vec2f ndc;
		int corner = -1;   // loop index of the face corner this is, or -1 if it was created by the near clip
		int edge = -1;     // loop index of the edge from this vertex to the next, or -1 if that part is the near clip
		float ua = 0.0f;   // parameter on `edge` at this vertex
		float ub = 1.0f;   // parameter on `edge` at the next vertex
	};

	struct CutVertex {
		enum Kind { EXISTING, ON_EDGE, INTERIOR } kind = EXISTING;
		uint vert = MAX_UINT;  // mesh vertex (EXISTING always, ON_EDGE after splitting)
		uint edge = MAX_UINT;  // ON_EDGE: canonical half-edge (lower id of the pair)
		float u = 0.0f;        // ON_EDGE: parameter along the canonical half-edge
		Vec3f world = Vec3f(0.0f);
		int visible = -1;
		bool used = false;
	};

	struct Chord {
		uint a, b;      // cut vertices
		Vec2f sa, sb;   // their screen positions
	};

	struct FacePlan {
		bool valid = false;
		Vec3f origin = Vec3f(0.0f);
		Vec3f normal = Vec3f(0.0f);
		Vec3f ax = Vec3f(0.0f), ay = Vec3f(0.0f);
		std::vector<Chord> chords;
	};

	HalfEdgeMesh& mesh;

	// State of the cut in progress
	KnifeCamera cam;
	const KnifeOccluders* occluders = nullptr;
	bool cutThrough = false;
	std::vector<Vec2f> path;
	std::vector<char> segmentHit; // per segment of `path`, whether it made a chord
	std::vector<Vec3f> world;     // world position of every mesh vertex (kept in step with the mesh)
	std::vector<CutVertex> cuts;
	std::vector<FacePlan> plans;
	std::unordered_map<uint, uint> existingCuts;               // vertex -> cut
	std::unordered_map<uint, std::vector<uint>> edgeCuts;      // canonical half-edge -> cuts
	std::unordered_map<unsigned long long, uint> interiorCuts; // (face, path index) -> cut

	static float Cross2(const Vec2f& a, const Vec2f& b) { return a.x * b.y - a.y * b.x; }

	/// Counterclockwise angle from a to b in [0, 2pi)
	static float CcwAngle(const Vec2f& a, const Vec2f& b) {
		float ang = std::atan2(Cross2(a, b), a.Dot(b));
		if (ang < 0.0f) { ang += 6.28318530718f; }
		return ang;
	}

	ClipPos Project(const Vec3f& p) const {
		const Matrix4f& m = cam.viewProj;
		return {
			m(0, 0) * p.x + m(0, 1) * p.y + m(0, 2) * p.z + m(0, 3),
			m(1, 0) * p.x + m(1, 1) * p.y + m(1, 2) * p.z + m(1, 3),
			m(2, 0) * p.x + m(2, 1) * p.y + m(2, 2) * p.z + m(2, 3),
			m(3, 0) * p.x + m(3, 1) * p.y + m(3, 2) * p.z + m(3, 3)
		};
	}

	static ClipPos Lerp(const ClipPos& a, const ClipPos& b, float t) {
		return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t };
	}

	Vec2f P2(const FacePlan& plan, const Vec3f& p) const {
		const Vec3f d = p - plan.origin;
		return Vec2f(d.Dot(plan.ax), d.Dot(plan.ay));
	}

	/// The point where the ray through a screen position meets a face's plane.
	bool LiftToPlane(const Vec2f& ndc, const FacePlan& plan, Vec3f& out) const {
		const Matrix4f& m = cam.viewProj;
		// x*w = clip.x and y*w = clip.y are two planes. Their intersection is the camera ray.
		float pa[4], pb[4];
		for (int c = 0; c < 4; ++c) {
			pa[c] = m(0, c) - ndc.x * m(3, c);
			pb[c] = m(1, c) - ndc.y * m(3, c);
		}
		const Vec3f na(pa[0], pa[1], pa[2]), nb(pb[0], pb[1], pb[2]);
		const float ca = pa[3], cb = pb[3];
		const Vec3f nc = plan.normal;
		const float cc = -nc.Dot(plan.origin);

		const float det = na.Dot(nb.Cross(nc));
		if (std::abs(det) < 1e-12f) { return false; }
		out = -(nb.Cross(nc) * ca + nc.Cross(na) * cb + na.Cross(nb) * cc) / det;

		// Behind the camera is no good
		return Project(out).w > 0.0f;
	}

	// -----------------------------------------------------------------------------------------------------------
	// Cut vertex registry
	// -----------------------------------------------------------------------------------------------------------

	uint ExistingCut(uint vert) {
		auto it = existingCuts.find(vert);
		if (it != existingCuts.end()) { return it->second; }
		CutVertex cut;
		cut.kind = CutVertex::EXISTING;
		cut.vert = vert;
		cut.world = world[vert];
		cuts.push_back(cut);
		return existingCuts[vert] = static_cast<uint>(cuts.size() - 1);
	}

	/// <param name="halfEdge">Any half-edge of the edge.</param>
	/// <param name="u">Parameter along `halfEdge` from its origin.</param>
	uint EdgeCut(uint halfEdge, float u) {
		const uint twin = mesh.edges[halfEdge].twin;
		uint canon = halfEdge;
		if (twin < halfEdge) { canon = twin; u = 1.0f - u; }

		std::vector<uint>& list = edgeCuts[canon];
		for (uint id : list) {
			if (std::abs(cuts[id].u - u) < EDGE_EPS) { return id; }
		}

		const HalfEdgeMesh::HalfEdge& e = mesh.edges[canon];
		CutVertex cut;
		cut.kind = CutVertex::ON_EDGE;
		cut.edge = canon;
		cut.u = u;
		cut.world = world[e.orig] * (1.0f - u) + world[e.dest] * u;
		cuts.push_back(cut);
		list.push_back(static_cast<uint>(cuts.size() - 1));
		return list.back();
	}

	uint InteriorCut(uint face, uint pathIdx) {
		const unsigned long long key = (static_cast<unsigned long long>(face) << 32) | pathIdx;
		auto it = interiorCuts.find(key);
		if (it != interiorCuts.end()) { return it->second; }

		Vec3f pos;
		if (!LiftToPlane(path[pathIdx], plans[face], pos)) { return MAX_UINT; }

		CutVertex cut;
		cut.kind = CutVertex::INTERIOR;
		cut.world = pos;
		cuts.push_back(cut);
		return interiorCuts[key] = static_cast<uint>(cuts.size() - 1);
	}

	bool IsVisible(uint cut) {
		CutVertex& c = cuts[cut];
		if (c.visible < 0) { c.visible = occluders->Blocked(cam.position, c.world) ? 0 : 1; }
		return c.visible == 1;
	}

	// -----------------------------------------------------------------------------------------------------------
	// Step 1: chords inside faces
	// -----------------------------------------------------------------------------------------------------------

	struct Crossing {
		float t;   // along the knife line
		float s;   // along the polygon edge
		int edge;  // polygon edge index
	};

	void CollectChords(uint face) {
		std::vector<uint> loop;
		{
			const uint first = mesh.faceEdges[face];
			uint cur = first;
			do { loop.push_back(cur); cur = mesh.edges[cur].next; } while (cur != first && loop.size() <= mesh.edges.size());
		}
		const int n = static_cast<int>(loop.size());
		if (n < 3) { return; }

		// Face plane, Newell's method
		FacePlan& plan = plans[face];
		{
			Vec3f nrm(0.0f);
			for (int k = 0; k < n; ++k) {
				const Vec3f& a = world[mesh.edges[loop[k]].orig];
				const Vec3f& b = world[mesh.edges[loop[(k + 1) % n]].orig];
				nrm.x += (a.y - b.y) * (a.z + b.z);
				nrm.y += (a.z - b.z) * (a.x + b.x);
				nrm.z += (a.x - b.x) * (a.y + b.y);
			}
			if (nrm.LengthSquared() < 1e-20f) { return; }
			nrm.Normalize();
			plan.normal = nrm;
			plan.origin = world[mesh.edges[loop[0]].orig];
			Vec3f helper = std::abs(nrm.x) < 0.9f ? Vec3f(1.0f, 0.0f, 0.0f) : Vec3f(0.0f, 1.0f, 0.0f);
			plan.ax = nrm.Cross(helper);
			plan.ax.Normalize();
			plan.ay = nrm.Cross(plan.ax);
		}

		// Face polygon on screen, clipped to the near plane
		std::vector<ClipPos> clip(n);
		std::vector<float> nd(n);
		for (int k = 0; k < n; ++k) {
			clip[k] = Project(world[mesh.edges[loop[k]].orig]);
			nd[k] = clip[k].z + clip[k].w;
		}

		std::vector<PolyVert> poly;
		for (int k = 0; k < n; ++k) {
			const int j = (k + 1) % n;
			const bool in0 = nd[k] > 0.0f, in1 = nd[j] > 0.0f;
			if (in0) {
				PolyVert v;
				v.clip = clip[k]; v.corner = k; v.edge = k; v.ua = 0.0f;
				v.ub = in1 ? 1.0f : nd[k] / (nd[k] - nd[j]);
				poly.push_back(v);
				if (!in1) {
					PolyVert w;
					w.clip = Lerp(clip[k], clip[j], v.ub);
					poly.push_back(w);
				}
			}
			else if (in1) {
				const float um = nd[k] / (nd[k] - nd[j]);
				PolyVert w;
				w.clip = Lerp(clip[k], clip[j], um);
				w.edge = k; w.ua = um; w.ub = 1.0f;
				poly.push_back(w);
			}
		}
		const int pn = static_cast<int>(poly.size());
		if (pn < 3) { return; }

		float area2 = 0.0f;
		for (PolyVert& v : poly) { v.ndc = Vec2f(v.clip.x / v.clip.w, v.clip.y / v.clip.w); }
		for (int j = 0; j < pn; ++j) { area2 += Cross2(poly[j].ndc, poly[(j + 1) % pn].ndc); }
		if (std::abs(area2) < 1e-9f) { return; } // seen edge on

		// Turns a crossing of the polygon boundary into a cut vertex
		auto boundaryCut = [&](const Crossing& c) -> uint {
			const PolyVert& a = poly[c.edge];
			const PolyVert& b = poly[(c.edge + 1) % pn];
			if (a.edge < 0) { return MAX_UINT; } // the near clip is not a real edge

			const float len = (b.ndc - a.ndc).Length();
			if (c.s * len < SNAP && a.corner >= 0) { return ExistingCut(mesh.edges[loop[a.corner]].orig); }
			if ((1.0f - c.s) * len < SNAP && b.corner >= 0) { return ExistingCut(mesh.edges[loop[b.corner]].orig); }

			// Screen space is not linear along an edge in perspective, 1/w is
			const float lam = (c.s / b.clip.w) / ((1.0f - c.s) / a.clip.w + c.s / b.clip.w);
			const float u = a.ua + lam * (a.ub - a.ua);
			const HalfEdgeMesh::HalfEdge& he = mesh.edges[loop[a.edge]];
			if (u < EDGE_EPS) { return ExistingCut(he.orig); }
			if (u > 1.0f - EDGE_EPS) { return ExistingCut(he.dest); }
			return EdgeCut(loop[a.edge], u);
		};

		std::vector<char> side(pn);
		std::vector<Crossing> crossings;

		for (uint i = 0; i + 1 < path.size(); ++i) {
			const Vec2f p0 = path[i];
			const Vec2f r = path[i + 1] - p0;
			if (r.LengthSquared() < 1e-14f) { continue; }

			// Which side of the (infinite) knife line each polygon vertex is on. A vertex exactly on the line counts
			// as one side, so a line through a vertex still crosses an even number of times.
			for (int j = 0; j < pn; ++j) { side[j] = Cross2(r, poly[j].ndc - p0) >= 0.0f; }

			crossings.clear();
			for (int j = 0; j < pn; ++j) {
				const int k = (j + 1) % pn;
				if (side[j] == side[k]) { continue; }
				const Vec2f e = poly[k].ndc - poly[j].ndc;
				const float den = Cross2(e, r);
				if (std::abs(den) < 1e-20f) { continue; }
				const Vec2f w = p0 - poly[j].ndc;
				Crossing c;
				c.s = std::clamp(Cross2(w, r) / den, 0.0f, 1.0f);
				c.t = Cross2(w, e) / den;
				c.edge = j;
				crossings.push_back(c);
			}
			if (crossings.size() < 2) { continue; }
			std::sort(crossings.begin(), crossings.end(), [](const Crossing& a, const Crossing& b) { return a.t < b.t; });

			// Between crossings 0-1, 2-3, ... the line is inside the polygon
			for (size_t k = 0; k + 1 < crossings.size(); k += 2) {
				const Crossing& c0 = crossings[k];
				const Crossing& c1 = crossings[k + 1];
				const float ta = std::max(c0.t, 0.0f);
				const float tb = std::min(c1.t, 1.0f);
				if (tb - ta < 1e-6f) { continue; }

				const uint a = c0.t < 0.0f ? InteriorCut(face, i) : boundaryCut(c0);
				const uint b = c1.t > 1.0f ? InteriorCut(face, i + 1) : boundaryCut(c1);
				if (a == MAX_UINT || b == MAX_UINT || a == b) { continue; }

				if (!cutThrough) {
					const Vec3f mid = (cuts[a].world + cuts[b].world) * 0.5f;
					if (!IsVisible(a) || !IsVisible(b) || occluders->Blocked(cam.position, mid)) { continue; }
				}

				plan.chords.push_back({ a, b, p0 + r * ta, p0 + r * tb });
				segmentHit[i] = 1;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------
	// Step 2: chords that cross each other inside a face
	// -----------------------------------------------------------------------------------------------------------

	void ResolveCrossings(uint face) {
		FacePlan& plan = plans[face];
		const size_t count = plan.chords.size();
		if (count == 0) { return; }

		struct Split { float p; uint cut; };
		std::vector<std::vector<Split>> splits(count);
		std::vector<std::pair<Vec2f, uint>> made; // crossings created in this face

		for (size_t i = 0; i < count; ++i) {
			for (size_t j = i + 1; j < count; ++j) {
				const Chord& ci = plan.chords[i];
				const Chord& cj = plan.chords[j];
				if (ci.a == cj.a || ci.a == cj.b || ci.b == cj.a || ci.b == cj.b) { continue; }

				const Vec2f e1 = ci.sb - ci.sa, e2 = cj.sb - cj.sa;
				const float l1 = e1.Length(), l2 = e2.Length();
				const float den = Cross2(e1, e2);
				if (std::abs(den) < 1e-9f * l1 * l2) { continue; } // parallel

				const Vec2f w = cj.sa - ci.sa;
				const float pi = Cross2(w, e2) / den;
				const float pj = Cross2(w, e1) / den;
				const float ei = SNAP / l1, ej = SNAP / l2;
				if (pi < -ei || pi > 1.0f + ei || pj < -ej || pj > 1.0f + ej) { continue; }

				const uint endI = pi < ei ? ci.a : (pi > 1.0f - ei ? ci.b : MAX_UINT);
				const uint endJ = pj < ej ? cj.a : (pj > 1.0f - ej ? cj.b : MAX_UINT);

				if (endI != MAX_UINT && endJ != MAX_UINT) { continue; }
				if (endI != MAX_UINT) { splits[j].push_back({ pj, endI }); continue; }
				if (endJ != MAX_UINT) { splits[i].push_back({ pi, endJ }); continue; }

				const Vec2f at = ci.sa + e1 * pi;
				uint cut = MAX_UINT;
				for (const auto& m : made) {
					if ((m.first - at).Length() < SNAP) { cut = m.second; break; }
				}
				if (cut == MAX_UINT) {
					Vec3f pos;
					if (!LiftToPlane(at, plan, pos)) { continue; }
					CutVertex c;
					c.kind = CutVertex::INTERIOR;
					c.world = pos;
					cuts.push_back(c);
					cut = static_cast<uint>(cuts.size() - 1);
					made.push_back({ at, cut });
				}
				splits[i].push_back({ pi, cut });
				splits[j].push_back({ pj, cut });
			}
		}

		std::vector<Chord> result;
		std::unordered_set<unsigned long long> seen;
		auto addChord = [&](uint a, uint b, const Vec2f& sa, const Vec2f& sb) {
			if (a == b) { return; }
			const unsigned long long key = (static_cast<unsigned long long>(std::min(a, b)) << 32) | std::max(a, b);
			if (!seen.insert(key).second) { return; }
			result.push_back({ a, b, sa, sb });
		};

		for (size_t i = 0; i < count; ++i) {
			const Chord& c = plan.chords[i];
			std::sort(splits[i].begin(), splits[i].end(), [](const Split& x, const Split& y) { return x.p < y.p; });
			uint prev = c.a;
			Vec2f prevPos = c.sa;
			const Vec2f e = c.sb - c.sa;
			for (const Split& s : splits[i]) {
				const Vec2f pos = c.sa + e * s.p;
				addChord(prev, s.cut, prevPos, pos);
				prev = s.cut;
				prevPos = pos;
			}
			addChord(prev, c.b, prevPos, c.sb);
		}
		plan.chords = std::move(result);
	}

	// -----------------------------------------------------------------------------------------------------------
	// Step 3: split the edges the cut crosses
	// -----------------------------------------------------------------------------------------------------------

	void SplitEdges() {
		for (auto& entry : edgeCuts) {
			std::vector<uint> ids;
			for (uint id : entry.second) { if (cuts[id].used) { ids.push_back(id); } }
			if (ids.empty()) { continue; }
			std::sort(ids.begin(), ids.end(), [&](uint a, uint b) { return cuts[a].u < cuts[b].u; });

			const uint h = entry.first;
			const Vec3f local0 = mesh.vertPos[mesh.edges[h].orig];
			const Vec3f local1 = mesh.vertPos[mesh.edges[h].dest];

			// Split from the far end, so h always remains the piece that starts at the original origin
			float limit = 1.0f;
			for (size_t k = ids.size(); k-- > 0;) {
				CutVertex& cut = cuts[ids[k]];
				const uint v = mesh.refineEdge(h, cut.u / limit);
				mesh.SetVertexPosition(v, local0 * (1.0f - cut.u) + local1 * cut.u);
				world.push_back(cut.world);
				cut.vert = v;
				limit = cut.u;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------
	// Step 4: build the topology
	// -----------------------------------------------------------------------------------------------------------

	/// The corner (outgoing half-edge) at `vert`, among the faces in `pieces`, whose wedge contains `dir`.
	uint FindCorner(const FacePlan& plan, uint vert, const Vec2f& dir, const std::vector<uint>& pieces) const {
		const std::vector<HalfEdgeMesh::HalfEdge>& edges = mesh.edges;
		const Vec2f at = P2(plan, world[vert]);
		const uint start = mesh.vertEdge[vert];
		uint cur = start;
		uint guard = 0;
		do {
			const uint f = edges[cur].face;
			if (f != MAX_UINT && std::find(pieces.begin(), pieces.end(), f) != pieces.end()) {
				const Vec2f dOut = P2(plan, world[edges[cur].dest]) - at;
				const Vec2f dIn = P2(plan, world[edges[edges[cur].prev].orig]) - at;
				float wedge = CcwAngle(dOut, dIn);
				if (wedge < 1e-6f) { wedge = 6.28318530718f; } // a lone spur sees all the way around
				const float ang = CcwAngle(dOut, dir);
				if (ang > 1e-6f && ang < wedge - 1e-6f) { return cur; }
			}
			cur = edges[edges[cur].twin].next;
		} while (cur != start && ++guard < edges.size());
		return MAX_UINT;
	}

	/// Make-edge-vertex: adds a vertex joined to the corner's vertex by a dangling edge, inside the corner.
	uint MakeEdgeVertex(uint corner, const Vec3f& pos) {
		const uint a = mesh.edges[corner].orig;
		const uint f = mesh.edges[corner].face;
		const uint in = mesh.edges[corner].prev;

		const uint v = mesh.NewVertex(mesh.WorldPosToLocalPos(pos), MAX_UINT);
		const uint e1 = mesh.NewEdge();
		const uint e2 = mesh.NewEdge();
		std::vector<HalfEdgeMesh::HalfEdge>& E = mesh.edges;

		E[e1] = HalfEdgeMesh::HalfEdge(a, v, f);
		E[e2] = HalfEdgeMesh::HalfEdge(v, a, f);
		E[e1].twin = e2; E[e2].twin = e1;

		E[in].next = e1;  E[e1].prev = in;
		E[e1].next = e2;  E[e2].prev = e1;
		E[e2].next = corner; E[corner].prev = e2;

		mesh.vertEdge[v] = e2;
		world.push_back(pos);
		return v;
	}

	/// Make-edge-face: joins two corners of the same face with an edge, splitting the face in two.
	/// <returns>The new face, or MAX_UINT if the corners can't be joined.</returns>
	uint MakeEdgeFace(uint cornerA, uint cornerB) {
		std::vector<HalfEdgeMesh::HalfEdge>& E = mesh.edges;
		if (cornerA == cornerB || E[cornerA].face != E[cornerB].face) { return MAX_UINT; }
		// Neighbors on the loop are already joined by an edge
		if (E[cornerA].next == cornerB || E[cornerB].next == cornerA) { return MAX_UINT; }

		const uint a = E[cornerA].orig, b = E[cornerB].orig;
		const uint f = E[cornerA].face;
		const uint inA = E[cornerA].prev, inB = E[cornerB].prev;

		const uint x = mesh.NewEdge();
		const uint y = mesh.NewEdge();
		E[x] = HalfEdgeMesh::HalfEdge(a, b, f);
		E[y] = HalfEdgeMesh::HalfEdge(b, a, f);
		E[x].twin = y; E[y].twin = x;

		// Loop one: inA -> x -> cornerB ... ; loop two: inB -> y -> cornerA ...
		E[inA].next = x;     E[x].prev = inA;
		E[x].next = cornerB; E[cornerB].prev = x;
		E[inB].next = y;     E[y].prev = inB;
		E[y].next = cornerA; E[cornerA].prev = y;

		const uint g = mesh.NewFace(y);
		uint cur = y;
		do { E[cur].face = g; cur = E[cur].next; } while (cur != y);
		mesh.faceEdges[f] = x;
		return g;
	}

	struct Node {
		uint cut;
		uint vert;
		Vec2f p;
		bool attached;
	};

	void InsertChords(uint face) {
		FacePlan& plan = plans[face];
		if (plan.chords.empty()) { return; }

		std::vector<Node> nodes;
		std::unordered_map<uint, uint> nodeOf;
		auto nodeFor = [&](uint cut) {
			auto it = nodeOf.find(cut);
			if (it != nodeOf.end()) { return it->second; }
			const CutVertex& c = cuts[cut];
			const bool onBoundary = c.kind != CutVertex::INTERIOR;
			nodes.push_back({ cut, onBoundary ? c.vert : MAX_UINT, P2(plan, c.world), onBoundary });
			return nodeOf[cut] = static_cast<uint>(nodes.size() - 1);
		};

		std::vector<std::pair<uint, uint>> pending;
		std::unordered_set<unsigned long long> seen;
		for (const Chord& c : plan.chords) {
			const uint a = nodeFor(c.a), b = nodeFor(c.b);
			if (a == b) { continue; }
			const unsigned long long key = (static_cast<unsigned long long>(std::min(a, b)) << 32) | std::max(a, b);
			if (seen.insert(key).second) { pending.push_back({ a, b }); }
		}

		// A group of chords that never reaches the face boundary can't be represented (a face has one loop). Drop it.
		std::vector<uint> parent(nodes.size());
		for (uint i = 0; i < parent.size(); ++i) { parent[i] = i; }
		auto find = [&](uint x) { while (parent[x] != x) { x = parent[x] = parent[parent[x]]; } return x; };
		for (const auto& e : pending) { parent[find(e.first)] = find(e.second); }
		std::vector<char> reachesBoundary(nodes.size(), 0);
		for (uint i = 0; i < nodes.size(); ++i) { if (nodes[i].attached) { reachesBoundary[find(i)] = 1; } }
		pending.erase(std::remove_if(pending.begin(), pending.end(),
			[&](const auto& e) { return !reachesBoundary[find(e.first)]; }), pending.end());

		std::vector<uint> pieces = { face };

		// Each pass adds every chord that has an end on the structure so far. Spurs come first from the boundary,
		// and loops close once both ends are on it.
		bool progress = true;
		while (!pending.empty() && progress) {
			progress = false;
			for (size_t i = 0; i < pending.size();) {
				uint a = pending[i].first, b = pending[i].second;
				if (!nodes[a].attached && !nodes[b].attached) { ++i; continue; }
				if (!nodes[a].attached) { std::swap(a, b); }

				bool done = false;
				const uint cornerA = FindCorner(plan, nodes[a].vert, nodes[b].p - nodes[a].p, pieces);
				if (cornerA != MAX_UINT) {
					if (!nodes[b].attached) {
						nodes[b].vert = MakeEdgeVertex(cornerA, cuts[nodes[b].cut].world);
						nodes[b].attached = true;
						done = true;
					}
					else {
						const uint cornerB = FindCorner(plan, nodes[b].vert, nodes[a].p - nodes[b].p, pieces);
						if (cornerB != MAX_UINT) {
							const uint g = MakeEdgeFace(cornerA, cornerB);
							if (g != MAX_UINT) { pieces.push_back(g); }
						}
						done = true; // either made, or the chord was already an edge
					}
				}
				else {
					done = true; // lies along an existing edge
				}

				if (done) {
					pending.erase(pending.begin() + i);
					progress = true;
				}
				else { ++i; }
			}
		}
	}

public:
	std::string error;

	HalfEdgeMeshKnife(HalfEdgeMesh& mesh) : mesh(mesh) {}

	/// <summary>
	/// Cuts the mesh along a path drawn on the screen.
	/// </summary>
	/// <param name="camera">The view the path was drawn in.</param>
	/// <param name="screenPath">The path's points in normalized device coordinates (-1 to 1, y up). At least 2.</param>
	/// <param name="cutThroughAll">When true, hidden geometry is cut too.</param>
	/// <param name="hidden">What can hide the mesh. Ignored when cutting through.</param>
	/// <param name="segmentsCut">
	/// Optional. Sized to one entry per segment of `screenPath`; entries are set to 1 where that segment cuts the
	/// mesh and left alone elsewhere, so several meshes can share one array.
	/// </param>
	/// <returns>True if the mesh was changed. On false the mesh is exactly as it was.</returns>
	bool Cut(const KnifeCamera& camera, const std::vector<Vec2f>& screenPath, bool cutThroughAll, const KnifeOccluders& hidden,
		std::vector<char>* segmentsCut = nullptr) {
		error.clear();
		if (segmentsCut) { segmentsCut->resize(screenPath.size() < 2 ? 0 : screenPath.size() - 1, 0); }
		if (screenPath.size() < 2 || mesh.FaceCount() == 0) { return false; }

		cam = camera;
		// Points on top of each other (a double click) would be two separate vertices in the same spot
		path.clear();
		std::vector<size_t> source; // index in screenPath of each kept point
		for (size_t k = 0; k < screenPath.size(); ++k) {
			if (path.empty() || (screenPath[k] - path.back()).Length() >= SNAP) {
				path.push_back(screenPath[k]);
				source.push_back(k);
			}
		}
		if (path.size() < 2) { return false; }
		segmentHit.assign(path.size() - 1, 0);
		cutThrough = cutThroughAll;
		occluders = &hidden;
		cuts.clear();
		existingCuts.clear();
		edgeCuts.clear();
		interiorCuts.clear();
		plans.assign(mesh.FaceCount(), FacePlan());

		const HalfEdgeMesh backup = mesh;

		world.clear();
		mesh.GetWorldVertexPositions(world);

		const uint faceCount = mesh.FaceCount();
		for (uint f = 0; f < faceCount; ++f) { CollectChords(f); }
		for (uint f = 0; f < faceCount; ++f) { ResolveCrossings(f); }

		if (segmentsCut) {
			// A kept segment stands for every original segment between its two points
			for (size_t i = 0; i < segmentHit.size(); ++i) {
				if (!segmentHit[i]) { continue; }
				for (size_t k = source[i]; k < source[i + 1]; ++k) { (*segmentsCut)[k] = 1; }
			}
		}

		bool any = false;
		for (uint f = 0; f < faceCount; ++f) {
			for (const Chord& c : plans[f].chords) {
				cuts[c.a].used = cuts[c.b].used = true;
				any = true;
			}
		}
		if (!any) { return false; }

		SplitEdges();
		for (uint f = 0; f < faceCount; ++f) { InsertChords(f); }

		std::string err;
		if (!mesh.Validate(err)) {
			error = "knife produced an invalid mesh: " + err;
			mesh = backup;
			return false;
		}
		return mesh.VertexCount() != backup.VertexCount() || mesh.EdgeCount() != backup.EdgeCount();
	}
};

#endif //_ABC3D_MESH_KNIFE_H
