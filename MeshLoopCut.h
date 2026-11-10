#ifndef _ABC3D_MESH_LOOP_CUT_H
#define _ABC3D_MESH_LOOP_CUT_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mesh.h"

/// <summary>
/// Loop cut: picks an edge, follows the strip of faces it belongs to, and splits every face in the
/// strip with new edges running alongside the picked edge.
///
/// The strip is found with an edge ring walk. Entering a face through one half-edge, the strip leaves through the
/// half-edge halfway around the face (n/2 steps along `next`), which is the edge that shares no vertex with the
/// entry. `twin` then crosses into the next face. The walk stops at
///   - an open boundary (an exterior half-edge, face == MAX_UINT),
///   - a face with an odd number of sides (triangles, pentagons...) or fewer than four,
///   - a face the strip has already passed through,
///   - or the starting edge, in which case the ring is closed.
/// When the ring is open, the walk is repeated in the other direction, starting from the picked edge's twin.
///
/// Every edge of the ring gets `cuts` evenly spaced vertices. Faces where the walk stopped keep the new vertex on
/// their side and are otherwise left alone, so a cut through a mesh made only of triangles just splits the edge.
///
/// Cut() never leaves the mesh half-modified: the result is validated and the mesh is restored if it is not valid.
/// </summary>
class HalfEdgeMeshLoopCutter {
public:
	/// <summary>A face of the strip, with the half-edges (of that face) the strip enters and leaves through.</summary>
	struct StripFace {
		uint face;
		uint entry;
		uint exit;
	};

	/// <summary>The faces and edges a loop cut through one edge would touch.</summary>
	struct Ring {
		std::vector<uint> edges;       // one half-edge per crossed edge, each edge once
		std::vector<StripFace> faces;  // faces that get split
		bool closed = false;           // true if the strip goes all the way around
	};

	std::string error;

	HalfEdgeMeshLoopCutter(HalfEdgeMesh& mesh) : mesh(mesh) {}

	/// <summary>Finds the edge ring and face strip through an edge, without changing anything.</summary>
	/// <param name="halfEdge">Either half-edge of the picked edge.</param>
	Ring FindRing(uint halfEdge) const {
		Ring ring;
		const std::vector<HalfEdgeMesh::HalfEdge>& E = mesh.edges;
		if (halfEdge >= E.size() || E[halfEdge].orig == MAX_UINT || E[halfEdge].twin >= E.size()) { return ring; }

		std::unordered_set<uint> visitedFaces;
		std::unordered_set<uint> visitedEdges;
		visitedEdges.insert(Canonical(halfEdge));

		// Returns true if the walk came back to where it started
		auto walk = [&](uint start, std::vector<StripFace>& faces, std::vector<uint>& edges) {
			uint cur = start;
			// Each pass visits a new face, so the walk can't take more steps than there are faces
			for (size_t guard = 0; guard <= mesh.faceEdges.size(); ++guard) {
				const uint f = E[cur].face;
				if (f == MAX_UINT || visitedFaces.count(f)) { return false; }

				const uint n = mesh.FaceEdgeCount(f);
				if (n < 4 || n % 2 != 0) { return false; }

				uint opp = cur;
				for (uint i = 0; i < n / 2; ++i) { opp = E[opp].next; }

				visitedFaces.insert(f);
				faces.push_back({ f, cur, opp });

				if (!visitedEdges.insert(Canonical(opp)).second) {
					return Canonical(opp) == Canonical(halfEdge);
				}
				edges.push_back(opp);
				cur = E[opp].twin;
			}
			return false;
		};

		std::vector<StripFace> forwardFaces, backwardFaces;
		std::vector<uint> forwardEdges, backwardEdges;
		ring.closed = walk(halfEdge, forwardFaces, forwardEdges);
		if (!ring.closed) { walk(E[halfEdge].twin, backwardFaces, backwardEdges); }

		// Order everything along the strip: backward part reversed, the picked edge, then the forward part
		ring.edges.assign(backwardEdges.rbegin(), backwardEdges.rend());
		ring.edges.push_back(halfEdge);
		ring.edges.insert(ring.edges.end(), forwardEdges.begin(), forwardEdges.end());
		ring.faces.assign(backwardFaces.rbegin(), backwardFaces.rend());
		ring.faces.insert(ring.faces.end(), forwardFaces.begin(), forwardFaces.end());
		return ring;
	}

	/// <summary>
	/// The new edges a cut would make, in the mesh's local space, for drawing a preview.
	/// </summary>
	/// <param name="halfEdge">Either half-edge of the picked edge.</param>
	/// <param name="cuts">Number of parallel cuts.</param>
	/// <param name="segments">Receives one pair of end points per new edge.</param>
	/// <param name="points">Receives every new vertex position.</param>
	void Preview(uint halfEdge, uint cuts, std::vector<std::pair<Vec3f, Vec3f>>& segments, std::vector<Vec3f>& points) const {
		segments.clear();
		points.clear();
		if (cuts == 0) { return; }
		const Ring ring = FindRing(halfEdge);
		if (ring.edges.empty()) { return; }

		const std::vector<HalfEdgeMesh::HalfEdge>& E = mesh.edges;
		const std::vector<Vec3f>& P = mesh.vertPos;
		auto along = [&](uint he, float s) { return P[E[he].orig] * (1.0f - s) + P[E[he].dest] * s; };

		for (uint he : ring.edges) {
			for (uint k = 0; k < cuts; ++k) { points.push_back(along(he, Param(k, cuts))); }
		}
		for (const StripFace& sf : ring.faces) {
			for (uint k = 0; k < cuts; ++k) {
				segments.push_back({ along(sf.entry, Param(k, cuts)), along(sf.exit, Param(cuts - 1 - k, cuts)) });
			}
		}
	}

	/// <summary>
	/// Cuts the strip through an edge.
	/// </summary>
	/// <param name="halfEdge">Either half-edge of the picked edge.</param>
	/// <param name="cuts">Number of parallel cuts, evenly spaced along each crossed edge.</param>
	/// <returns>True if the mesh was changed. On false the mesh is exactly as it was.</returns>
	bool Cut(uint halfEdge, uint cuts = 1) {
		error.clear();
		if (cuts == 0) { return false; }

		const Ring ring = FindRing(halfEdge);
		if (ring.edges.empty()) {
			error = "loop cut: not a valid edge";
			return false;
		}

		const HalfEdgeMesh backup = mesh;

		// Record which way each strip face's entry and exit run along their edge, before splitting changes twins.
		// After a split, the canonical half-edge id still starts at the edge's original origin.
		struct FacePlan {
			uint entryEdge; bool entryForward;
			uint exitEdge;  bool exitForward;
		};
		std::vector<FacePlan> plans;
		plans.reserve(ring.faces.size());
		for (const StripFace& sf : ring.faces) {
			plans.push_back({ Canonical(sf.entry), Canonical(sf.entry) == sf.entry,
			                  Canonical(sf.exit),  Canonical(sf.exit) == sf.exit });
		}

		// Step 1: split every crossed edge. New vertices are listed in order along the canonical half-edge.
		std::unordered_map<uint, std::vector<uint>> edgeVerts;
		for (uint he : ring.edges) {
			const uint canon = Canonical(he);
			std::vector<uint>& verts = edgeVerts[canon];
			verts.assign(cuts, MAX_UINT);

			// Split from the far end, so `canon` always remains the piece that starts at the original origin
			float limit = 1.0f;
			for (uint k = cuts; k-- > 0;) {
				const float s = Param(k, cuts);
				verts[k] = mesh.refineEdge(canon, s / limit);
				limit = s;
			}
		}

		// Step 2: join the new vertices across each face of the strip
		for (const FacePlan& plan : plans) {
			const std::vector<uint>& in = edgeVerts[plan.entryEdge];
			const std::vector<uint>& out = edgeVerts[plan.exitEdge];
			for (uint k = 0; k < cuts; ++k) {
				// Position along the face's own half-edge. Opposite sides of the face run in opposite directions,
				// so the k-th vertex along the entry lines up with the k-th from the end along the exit.
				const uint a = plan.entryForward ? in[k] : in[cuts - 1 - k];
				const uint kb = cuts - 1 - k;
				const uint b = plan.exitForward ? out[kb] : out[cuts - 1 - kb];

				uint cornerA, cornerB;
				if (!FindCorners(a, b, cornerA, cornerB) || MakeEdgeFace(cornerA, cornerB) == MAX_UINT) {
					error = "loop cut: could not split a face of the strip";
					mesh = backup;
					return false;
				}
			}
		}

		std::string err;
		if (!mesh.Validate(err)) {
			error = "loop cut produced an invalid mesh: " + err;
			mesh = backup;
			return false;
		}
		return true;
	}

private:
	HalfEdgeMesh& mesh;

	/// Where the k-th of `cuts` evenly spaced cuts sits along an edge, from 0 to 1
	static float Param(uint k, uint cuts) { return float(k + 1) / float(cuts + 1); }

	/// The lower id of a half-edge pair, used to name the edge
	uint Canonical(uint halfEdge) const {
		const uint twin = mesh.edges[halfEdge].twin;
		return twin < halfEdge ? twin : halfEdge;
	}

	/// Finds a face that both vertices are on, and the half-edges of that face leaving each of them.
	bool FindCorners(uint a, uint b, uint& cornerA, uint& cornerB) const {
		const std::vector<HalfEdgeMesh::HalfEdge>& E = mesh.edges;
		const uint start = mesh.vertEdge[a];
		if (start >= E.size()) { return false; }

		uint cur = start;
		for (size_t guard = 0; guard < E.size(); ++guard) {
			if (E[cur].face != MAX_UINT) {
				uint x = E[cur].next;
				for (size_t steps = 0; x != cur && steps < E.size(); ++steps) {
					if (E[x].orig == b) {
						cornerA = cur;
						cornerB = x;
						return true;
					}
					x = E[x].next;
				}
			}
			cur = E[E[cur].twin].next; // next half-edge leaving `a`
			if (cur == start) { break; }
		}
		return false;
	}

	/// Make-edge-face: joins two corners of the same face with an edge, splitting the face in two.
	/// <returns>The new face, or MAX_UINT if the corners can't be joined.</returns>
	uint MakeEdgeFace(uint cornerA, uint cornerB) {
		std::vector<HalfEdgeMesh::HalfEdge>& E = mesh.edges;
		if (cornerA == cornerB || E[cornerA].face != E[cornerB].face || E[cornerA].face == MAX_UINT) { return MAX_UINT; }
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
};

#endif //_ABC3D_MESH_LOOP_CUT_H
