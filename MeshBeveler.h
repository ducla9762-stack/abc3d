#ifndef _ABC3D_MESH_BEVELER_H_
#define _ABC3D_MESH_BEVELER_H_

#include "mesh.h"

/// <summary>
/// Performs beveling operations on a given Half-Edge Mesh.
/// </summary>
class HalfEdgeMeshBeveler {
	/// <summary>
	/// Represents a vertex mesh, a mesh that will replace an original vertex in
	/// a larger mesh.
	/// </summary>
	struct VMesh {
		uint vertexId; // original vertex in larger mesh
		Vec3f vertPos; // position of original vertex in larger mesh (used for extrusion only)
		uint id; // Used to associate the V Mesh with an edge/face in the final mesh (used for extrusion)

		uint patchEdgeCount; // number of edges (and vertices) in this vertex mesh
		uint degree; // number of incident edges of the original vertex

		Vec2u* incidentEdges; // outgoing edge ids and their new origin vertex
		bool* edgeIsBevel; // used to determine whether an outgoing edge is being beveled

		VMesh(uint vertexId, const Vec3f& vertPos) : vertexId(vertexId), vertPos(vertPos) { patchEdgeCount = 0; degree = 0; }
		~VMesh() {
			if (incidentEdges) { delete[] incidentEdges; }
			if (edgeIsBevel) { delete[] edgeIsBevel; }
		}
	};

	HalfEdgeMesh& mesh;

	// Vertex Tracks
	std::vector<uint> vertices; // each vertex's id
	std::vector<Vec3f> vertTransforms; // each vertex's start position and translation

	// Edge cache for edge/vertex extrusion
	std::vector<uint> elements;

	// Cached vertex meshes
	std::vector<VMesh> vertexMeshes;

	/// <summary>
	/// Builds a vertex mesh, which is a mesh that will replace a vertex when beveling
	/// incident edges. The vertex mesh's vertex ids will be relative.
	/// </summary>
	/// <param name="vMesh">
	/// Vertex mesh to build. Must have its vertexId member set. patchEdgeCount and degree must be 0.
	/// </param>
	/// <param name="edgeSelected">Hashmap to determine if an edge is selected.</param>
	void BuildVMesh(VMesh& vMesh, const std::unordered_map<uint, uint>& edgeSelected) {
		uint firstEdge = mesh.vertEdge[vMesh.vertexId];
		uint currEdge = firstEdge;

		// Count incident edges to determine degree
		do {
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[currEdge];
			++vMesh.degree;
			currEdge = mesh.edges[edge.twin].next;
		} while (currEdge != firstEdge);

		// Allocate memory
		vMesh.incidentEdges = new Vec2u[vMesh.degree];
		vMesh.edgeIsBevel = new bool[vMesh.degree];

		// Load adjacent edge id's and determine which are being beveled
		for (uint i = 0; i < vMesh.degree; ++i) {
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[currEdge];
			vMesh.incidentEdges[i] = Vec2u(currEdge, 0);
			vMesh.edgeIsBevel[i] = edgeSelected.count(currEdge);
			currEdge = mesh.edges[edge.twin].next;
		}

		// Assign each outgoing edge a new, relative origin vertex id
		for (uint i = 0; i < vMesh.degree - 1; ++i) {
			uint edgeId = vMesh.incidentEdges[i][0];
			vMesh.incidentEdges[i][1] = vMesh.patchEdgeCount;
			// The next outgoing edge (clockwise) will have a different
			// origin id from this one if this edge is being beveled or
			// if the next edge is not being beveled
			if (vMesh.edgeIsBevel[i] || !vMesh.edgeIsBevel[i + 1]) {
				++vMesh.patchEdgeCount;
			}
		}

		// Properly determine the new, relative origin vertex id of the last edge
		// by considering first (next edge by clockwise wrap around) and previous edge
		if (!vMesh.edgeIsBevel[vMesh.degree - 1] && vMesh.edgeIsBevel[0]) {
			vMesh.incidentEdges[vMesh.degree - 1][1] = 0;
		}
		else {
			vMesh.incidentEdges[vMesh.degree - 1][1] = vMesh.patchEdgeCount;
			++vMesh.patchEdgeCount;
		}
	}

	/// <summary>
	/// Helper function for the IncorporateVMesh function. Incorporates a vertex mesh
	/// that represents a single edge into the overall mesh (i.e. the original vertex
	/// in the mesh is replaced by an edge).
	/// </summary>
	/// <param name="vMesh">The vertex mesh representing a single edge to incorporate</param>
	/// <param name="bevelMap">
	/// Hash map to assign beveled edges a corresponding edge that
	///		1. is a twin to an edge in the vertex mesh
	///		2. will be the corresponding side of the beveled edge's bevel strip (edge.twin.next in final mesh)
	/// </param>
	void IncorporateVEdge(const VMesh& vMesh, std::unordered_map<uint, uint>& bevelMap) {
		if (vMesh.degree != 3 && vMesh.degree != 4) { return; }

		uint edgeId = mesh.NewEdge();
		uint twinId = mesh.NewEdge();

		// Degree 4 is specific edge case
		if (vMesh.degree == 4) {
			uint orig = vMesh.vertexId;
			uint dest = vMesh.incidentEdges[2][1];

			uint offset = !vMesh.edgeIsBevel[0];
			bevelMap[vMesh.incidentEdges[0 + offset][0]] = edgeId;
			bevelMap[vMesh.incidentEdges[2 + offset][0]] = twinId;

			mesh.SetTwins(edgeId, twinId, orig, dest);

			return;
		}

		// Degree 3
		uint bevelDegree = 0;
		uint bevelEdges[2] = { MAX_UINT, MAX_UINT };
		for (uint i = 0; i < 3; ++i) {
			if (vMesh.edgeIsBevel[i]) {
				bevelEdges[bevelDegree] = i;
				++bevelDegree;
			}
		}

		uint orig = vMesh.incidentEdges[bevelEdges[0]][1];
		uint dest = vMesh.incidentEdges[WrapInc(bevelEdges[0], 2)][1];
		mesh.SetTwins(edgeId, twinId, orig, dest);

		bevelMap[vMesh.incidentEdges[bevelEdges[0]][0]] = edgeId;

		if (bevelDegree == 1) {
			mesh.InsertPrev(vMesh.incidentEdges[WrapDec(bevelEdges[0], 2)][0], twinId);
		}
		else {
			bevelMap[vMesh.incidentEdges[bevelEdges[1]][0]] = twinId;
		}
	}

	/// <summary>
	/// Takes a vertex mesh and converts it's relative vertex ids
	/// to absolute ids representing new vertices in the overall mesh.
	/// </summary>
	/// <param name="vMesh">Vertex mesh to convert.</param>
	/// <param name="offset">
	/// The id to be associated with the first new vertex of this vertex mesh.
	/// </param>
	void RelativeToAbsolute(VMesh& vMesh, uint offset) {
		// 0 corresponds to vMesh.vertexId and 1... corresponds to offset...
		uint vertOffset = offset - 1;
		for (uint i = 0; i < vMesh.degree; ++i) {
			Vec2u& edge = vMesh.incidentEdges[i];
			if (edge[1] == 0) { edge[1] = vMesh.vertexId; }
			else { edge[1] += vertOffset; }
		}
	}

	/// <summary>
	/// Sets the vertex tracks for all vertices of a vertex mesh.
	/// Each track constitutes a start location and a displacement.
	/// </summary>
	/// <param name="vMesh">The vertex mesh whose vertices need tracks.</param>
	void SetVertexTracks(VMesh& vMesh) {
		uint prevEdge = vMesh.incidentEdges[vMesh.degree - 1][0];
		bool prevWasBevel = vMesh.edgeIsBevel[vMesh.degree - 1];

		const Vec3f& vertPos = mesh.vertPos[vMesh.vertexId];
		for (uint i = 0; i < vMesh.degree; ++i) {
			Vec2u& edgeIds = vMesh.incidentEdges[i];

			if (!vMesh.edgeIsBevel[i]) {
				vertices.push_back(edgeIds[1]);
				vertTransforms.push_back(vertPos);
				vertTransforms.push_back(mesh.vertPos[mesh.edges[edgeIds[0]].dest] - vertPos);
				vertTransforms.back() *= 0.5f;
				// vert edgeIds[1] slides down edgeIds[0]
			}
			else if (prevWasBevel) {
				vertices.push_back(edgeIds[1]);
				vertTransforms.push_back(vertPos);
				vertTransforms.push_back(mesh.vertPos[mesh.edges[edgeIds[0]].dest] - vertPos);
				vertTransforms.back() += mesh.vertPos[mesh.edges[prevEdge].dest] - vertPos;
				vertTransforms.back() *= 0.5f;
				// vert edgeIds[1] slides down edgeIds[0] and prevEdge
			}

			prevWasBevel = vMesh.edgeIsBevel[i];
			prevEdge = edgeIds[0];
		}
	}

	/// <summary>
	/// Incorporates a vertex mesh into the overall mesh (i.e. the original vertex in the
	/// overall mesh is replaced by the vertex mesh)
	/// </summary>
	/// <param name="vMesh">The vertex mesh replacing an original vertex in the overall mesh</param>
	/// <param name="bevelMap">
	/// Hash map to assign beveled edges a corresponding edge that
	///		1. is a twin to an edge in the vertex mesh
	///		2. will be the corresponding side of the beveled edge's bevel strip (edge.twin.next in final mesh)
	/// </param>
	void IncorporateVMesh(VMesh& vMesh, std::unordered_map<uint, uint>& bevelMap) {
		mesh.vertPos.insert(mesh.vertPos.end(), vMesh.patchEdgeCount - 1, mesh.vertPos[vMesh.vertexId]);
		mesh.vertEdge.insert(mesh.vertEdge.end(), vMesh.patchEdgeCount - 1, 0);

		// Set Each Incident Edge's New Vertex (since this vertex is becoming a junction face/edge)
		for (uint i = 0; i < vMesh.degree; ++i) {
			const Vec2u& edgeIds = vMesh.incidentEdges[i];
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeIds[0]];

			uint vertexId = edgeIds[1];

			edge.orig = vertexId;

			if (vMesh.edgeIsBevel[i]) {
				mesh.edges[edge.twin].dest = vMesh.incidentEdges[WrapInc(i, vMesh.degree - 1)][1];
			}
			else {
				mesh.edges[edge.twin].dest = vertexId;
			}

			mesh.vertEdge[vertexId] = edgeIds[0]; // set vertEdge for this vert (may happen multiple times but that's fine)
		}

		// If junction is a single edge
		if (vMesh.patchEdgeCount <= 2) {
			IncorporateVEdge(vMesh, bevelMap);
			return;
		}

		// Build Junction Face
		uint firstNewEdge = mesh.edges.size();
		uint juncFace = mesh.NewFace(firstNewEdge);
		vMesh.id = juncFace;

		uint edgeIndex = 0;
		uint prevJunctionVertId = vMesh.incidentEdges[vMesh.degree - 1][1];
		bool prevWasBevel = vMesh.edgeIsBevel[vMesh.degree - 1];
		for (uint i = 0; i < vMesh.degree; ++i) {
			const Vec2u& edgeIds = vMesh.incidentEdges[i];

			if (prevWasBevel && !vMesh.edgeIsBevel[i]) {
				prevJunctionVertId = edgeIds[1];
				prevWasBevel = false;
				continue;
			}

			uint juncEdgeOrig;
			uint juncEdgeDest;

			if (vMesh.edgeIsBevel[i]) {
				juncEdgeOrig = vMesh.incidentEdges[WrapInc(i, vMesh.degree - 1)][1];
				juncEdgeDest = edgeIds[1];
			}
			else {
				juncEdgeOrig = edgeIds[1];
				juncEdgeDest = prevJunctionVertId;
			}

			// Create new edge from prevJunctionVertId to currJunctionVertId
			uint juncEdgeId = mesh.NewEdge();
			uint juncTwinId = mesh.NewEdge();
			mesh.SetTwins(juncEdgeId, juncTwinId, juncEdgeOrig, juncEdgeDest);

			HalfEdgeMesh::HalfEdge& juncEdge = mesh.edges[juncEdgeId];
			juncEdge.face = juncFace;
			juncEdge.next = firstNewEdge + (2 * WrapDec(edgeIndex, vMesh.patchEdgeCount - 1));
			juncEdge.prev = firstNewEdge + (2 * WrapInc(edgeIndex, vMesh.patchEdgeCount - 1));
			++edgeIndex;

			if (vMesh.edgeIsBevel[i]) {
				// twin can be set by bevel face construction
				bevelMap[edgeIds[0]] = juncTwinId;
			}
			else {
				mesh.InsertPrev(edgeIds[0], juncTwinId);
			}

			prevJunctionVertId = edgeIds[1];
			prevWasBevel = vMesh.edgeIsBevel[i];
		}
	}

public:
	HalfEdgeMeshBeveler(HalfEdgeMesh& mesh) : mesh(mesh) {}
	void StartEdgeBevel(const std::vector<uint>& edges) {
		std::unordered_map<uint, uint> vertMap;
		std::unordered_map<uint, uint> edgeMap; // used for edge and vertex duplication

		uint reserveSize = edges.size() * 2;
		vertMap.reserve(reserveSize);
		edgeMap.reserve(reserveSize);
		vertices.reserve(reserveSize);
		vertTransforms.reserve(reserveSize * 2);

		vertexMeshes.reserve(reserveSize);
		elements.reserve(edges.size());

		// Create vertex meshes
		for (uint edgeId : edges) {
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeId];

			if (edgeMap.count(edgeId) || edgeMap.count(edge.twin)) { continue; }

			edgeMap[edgeId] = 0;
			edgeMap[edge.twin] = 0;

			elements.push_back(edgeId);

			if (!vertMap.count(edge.orig)) {
				vertMap[edge.orig] = 0;
				vertexMeshes.emplace_back(edge.orig, mesh.vertPos[edge.orig]);
			}

			if (!vertMap.count(edge.dest)) {
				vertMap[edge.dest] = 0;
				vertexMeshes.emplace_back(edge.dest, mesh.vertPos[edge.dest]);
			}
		}

		// Build Vertex Meshes
		uint currVertCount = mesh.vertPos.size();
		uint newVertices = 0;
		for (VMesh& vMesh : vertexMeshes) {
			BuildVMesh(vMesh, edgeMap); // build vmesh
			RelativeToAbsolute(vMesh, currVertCount + newVertices);
			newVertices += vMesh.patchEdgeCount - 1;
		}

		// Build vertex tracks
		for (VMesh& vMesh : vertexMeshes) {
			SetVertexTracks(vMesh);
		}

		// Incorporate vertex meshes
		mesh.vertPos.reserve(currVertCount + newVertices);
		mesh.vertEdge.reserve(currVertCount + newVertices);
		for (VMesh& vMesh : vertexMeshes) { IncorporateVMesh(vMesh, edgeMap); }

		// Build Bevel Strips
		for (uint hEdge1Id : elements) {

			uint hEdge2Id = mesh.edges[hEdge1Id].twin;
			uint side1 = edgeMap[hEdge1Id];
			uint side2 = edgeMap[hEdge2Id];
			uint face = mesh.NewFace(hEdge2Id);

			uint faceEdges[4] = { mesh.NewTwin(hEdge1Id, face), side1, mesh.NewTwin(hEdge2Id, face), side2 };
			mesh.faceEdges[face] = faceEdges[0];

			for (uint i = 0; i < 4; ++i) {
				HalfEdgeMesh::HalfEdge& edge = mesh.edges[faceEdges[i]];

				edge.next = faceEdges[WrapInc(i, 3)];
				edge.prev = faceEdges[WrapDec(i, 3)];
			}

			mesh.edges[side1].face = face;
			mesh.edges[side2].face = face;
		}
	}

	void FanEdge(uint edge, uint vert, uint face, uint& edge1Id, uint& edge2Id) {
		edge1Id = mesh.NewEdge();
		edge2Id = mesh.NewEdge();

		HalfEdgeMesh::HalfEdge& edge0 = mesh.edges[edge];
		edge0.next = edge1Id;
		edge0.prev = edge2Id;
		edge0.face = face;

		HalfEdgeMesh::HalfEdge& edge1 = mesh.edges[edge1Id];
		edge1.next = edge2Id;
		edge1.prev = edge;
		edge1.orig = edge0.dest;
		edge1.dest = vert;
		edge1.face = face;

		HalfEdgeMesh::HalfEdge& edge2 = mesh.edges[edge2Id];
		edge2.next = edge;
		edge2.prev = edge1Id;
		edge2.orig = vert;
		edge2.dest = edge0.orig;
		edge2.face = face;
	}

	void FanVertexMesh(const VMesh& vMesh) {
		uint curr = mesh.faceEdges[vMesh.id];
		uint vert = mesh.NewVertex(vMesh.vertPos, uint(mesh.edges.size() + 1));

		uint prevWall, firstWall;

		uint next = mesh.edges[curr].next;
		FanEdge(curr, vert, vMesh.id, prevWall, firstWall);
		curr = next;

		for (uint i = 1; i < vMesh.patchEdgeCount; ++i) {
			next = mesh.edges[curr].next;
			uint leftWall, rightWall;
			FanEdge(curr, vert, mesh.NewFace(curr), leftWall, rightWall);
			mesh.edges[prevWall].twin = rightWall;
			mesh.edges[rightWall].twin = prevWall;
			prevWall = leftWall;
			curr = next;
		}

		mesh.edges[firstWall].twin = prevWall;
		mesh.edges[prevWall].twin = firstWall;
	}

	void DeleteTriEdge(uint edgeId) {
		// right now edge and twin ids will be emtpy indices in the edge
		// list but we need to handle these deletions properly later
		HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeId];
		HalfEdgeMesh::HalfEdge& twin = mesh.edges[edge.twin];

		mesh.edges[edge.prev].next = twin.next;
		mesh.edges[twin.next].prev = edge.prev;

		mesh.edges[edge.next].prev = twin.prev;
		mesh.edges[twin.prev].next = edge.next;

		mesh.edges[edge.prev].face = twin.face;
		mesh.edges[edge.next].face = twin.face;

		mesh.faceEdges[edge.face] = MAX_UINT;
	}

	void IncorporateExtrudeJunction(const VMesh& vMesh) {
		if (vMesh.patchEdgeCount < 3) { return; } // handle later 

		FanVertexMesh(vMesh);

		uint curr = mesh.faceEdges[vMesh.id];
		uint prevVert = vMesh.incidentEdges[0][1];
		bool prevIsBevel = vMesh.edgeIsBevel[0];
		for (uint i = 1; i < vMesh.degree; ++i) {
			uint vert = vMesh.incidentEdges[i][1];

			if (vert == prevVert) {
				prevVert = vert;
				prevIsBevel = vMesh.edgeIsBevel[i];
				// don't update curr
				continue;
			}

			uint next = mesh.edges[mesh.edges[mesh.edges[curr].prev].twin].prev;

			if (prevIsBevel){ DeleteTriEdge(curr); }

			curr = next;
			prevVert = vert;
			prevIsBevel = vMesh.edgeIsBevel[i];
		}

		if (prevIsBevel) { DeleteTriEdge(curr); }
	}

	// Can call only after an edge bevel
	void ExtrudeBeveledEdges(std::vector<uint>& extrudedEdges) {
		extrudedEdges.reserve(extrudedEdges.size() + elements.size());

		for (const VMesh& vMesh : vertexMeshes) {
			IncorporateExtrudeJunction(vMesh);
		}

		for (uint beveledEdge : elements) {
			uint split1 = mesh.NewEdge();
			uint split2 = mesh.NewEdge();
			uint top = mesh.edges[beveledEdge].twin;
			uint bottom = mesh.NthNext<3>(top);

			for (int i = 0; i < 2; ++i) {
				HalfEdgeMesh::HalfEdge& edge = mesh.edges[top];
				HalfEdgeMesh::HalfEdge& prev = mesh.edges[edge.prev];
				HalfEdgeMesh::HalfEdge& next = mesh.edges[edge.next];

				prev.prev = split1;
				next.next = split1;
				
				HalfEdgeMesh::HalfEdge& split = mesh.edges[split1];
				split.orig = next.dest;
				split.dest = prev.orig;
				split.twin = split2;
				split.prev = edge.next;
				split.next = edge.prev;
				split.face = edge.face;

				std::swap(split1, split2);
				std::swap(top, bottom);
			}

			uint newFace = mesh.NewFace(bottom);

			for (int i = 0; i < 4; ++i) {
				HalfEdgeMesh::HalfEdge& edge = mesh.edges[bottom];
				edge.face = newFace;
				bottom = edge.next;
			}

			extrudedEdges.push_back(split1);
		}
	}

	void StartFaceBevel(const std::vector<uint>& faceIds) {
		std::vector<uint> edges;
		edges.reserve(faceIds.size() * 4); // rough estimate for number of edges
		for (uint face : faceIds) { mesh.GetFaceEdges(face, edges); }
		StartEdgeBevel(edges);
	}

	void StartObjectBevel() {
		std::vector<uint> edges;
		edges.reserve((mesh.edges.size() / 2) + 1);
		for (uint i = 0; i < mesh.edges.size(); ++i) {
			if (mesh.edges[i].twin > i) { edges.push_back(i); }
		}
		StartEdgeBevel(edges);
	}

	void AdjustBevel(float bevelFactor) {
		bevelFactor = std::min(bevelFactor, 1.0f);
		bevelFactor = std::max(bevelFactor, 0.0f);

		for (size_t i = 0; i < vertices.size(); ++i) {
			size_t transIndex = i * 2;
			mesh.vertPos[vertices[i]] = vertTransforms[transIndex] + (vertTransforms[transIndex + 1] * bevelFactor);
		}
	}

	void StartVertexBevel(const std::vector<uint>& vertexIds) {
		std::unordered_map<uint, uint> vertMap;

		vertMap.reserve(vertices.size());
		vertices.reserve(vertices.size());
		vertTransforms.reserve(vertices.size() * 2);

		uint newVertices = 0;
		uint newEdges = 0;

		for (uint vertex : vertexIds) {
			if (vertMap.count(vertex)) { continue; }

			uint degree = mesh.VertexDegree(vertex);
			newVertices += degree - 1;
			newEdges += degree;
			vertMap[vertex] = degree;
		}

		newEdges *= 2;

		mesh.vertPos.reserve(mesh.vertPos.size() + newVertices);
		mesh.vertEdge.reserve(mesh.vertEdge.size() + newVertices);
		mesh.edges.reserve(mesh.edges.size() + newEdges);

		for (uint vertex : vertexIds) {
			uint degree = vertMap[vertex];
			if (degree == MAX_UINT) { continue; }

			uint firstEdge = mesh.vertEdge[vertex];
			uint currEdge = mesh.edges[mesh.edges[firstEdge].twin].next;
			Vec3f vertPos = mesh.vertPos[vertex];

			uint firstNewEdge = mesh.edges.size();
			mesh.edges.insert(mesh.edges.end(), degree, HalfEdgeMesh::HalfEdge());

			uint vFace = mesh.NewFace(firstNewEdge);

			uint prevVert = vertex;
			for (uint i = 1; i < degree; ++i) {
				uint vFaceTwin = mesh.NewEdge();

				HalfEdgeMesh::HalfEdge& edge = mesh.edges[currEdge];

				edge.orig = mesh.NewVertex(vertPos, currEdge);
				mesh.edges[edge.twin].dest = edge.orig;

				uint vFaceEdgeId = firstNewEdge + i;
				HalfEdgeMesh::HalfEdge& vFaceEdge = mesh.edges[vFaceEdgeId];

				mesh.SetTwins(vFaceEdgeId, vFaceTwin, edge.orig, prevVert);
				mesh.InsertPrev(currEdge, vFaceTwin);

				vFaceEdge.face = vFace;
				vFaceEdge.prev = firstNewEdge + WrapInc(i, degree - 1, firstNewEdge);
				vFaceEdge.next = vFaceEdgeId - 1;

				vertices.push_back(edge.orig);
				vertTransforms.push_back(vertPos);
				vertTransforms.push_back(0.5f * (mesh.vertPos[edge.dest] - vertPos));

				prevVert = edge.orig;
				currEdge = mesh.edges[edge.twin].next;
			}

			uint vFaceTwin = mesh.NewEdge();
			HalfEdgeMesh::HalfEdge& vFaceEdge = mesh.edges[firstNewEdge];

			mesh.SetTwins(firstNewEdge, vFaceTwin, vertex, prevVert);
			mesh.InsertPrev(firstEdge, vFaceTwin);

			vFaceEdge.face = vFace;
			vFaceEdge.prev = firstNewEdge + 1;
			vFaceEdge.next = firstNewEdge + degree - 1;

			vertices.push_back(vertex);
			vertTransforms.push_back(vertPos);
			vertTransforms.push_back(0.5f * (mesh.vertPos[mesh.edges[firstEdge].dest] - vertPos));

			vertMap[vertex] = MAX_UINT;
		}
	}

	void EndBevel() {}
};

#endif