#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <limits>
#include <unordered_map>
#include "cyVector.h"
#include "cyMatrix.h"
#include "helper.h"

using std::string;
using std::ifstream;
using std::ostream;

typedef unsigned uint;

const uint MAX_UINT = std::numeric_limits<uint>::max();

class FaceList {

	typedef cy::Vec4<uint> Face;
	typedef cy::Vec3f Vertex;

	std::vector<Vertex> vertices;
	std::vector<Face> faces;

	void MoveTo(const char*& c_str, char delim);
	void ProcessObjLine(const string& line);
	void PrintFaceComponent(uint x) const;
	void PrintFace(const Face& face) const;

public:

	const std::vector<cy::Vec3f>& Vertices() const;
	const std::vector<cy::Vec4<uint>>& Faces() const;
	std::vector<cy::Vec3f>& Vertices();
	std::vector<cy::Vec4<uint>>& Faces();
	void PrintContents() const;
	bool LoadObj(const char* obj);
};

class HalfEdgeMesh {

	struct HalfEdge {

		HalfEdge(){}

		HalfEdge(uint orig, uint dest)
			: orig(orig), dest(dest) {}

		HalfEdge(uint orig, uint dest, uint face) 
			: orig(orig), dest(dest), face(face) {}

		uint orig = MAX_UINT;
		uint dest = MAX_UINT;

		uint face = MAX_UINT;

		uint twin = MAX_UINT;
		uint next = MAX_UINT;
		uint prev = MAX_UINT;
	};

	struct VertexAdjacentInfo {
		uint edgeCount = 0;
		uint quadCount = 0;
		uint triCount = 0;
	};

	uint NewFace(uint faceEdge);
	uint NewVertex(const cy::Vec3f& pos, uint firstEdgeId);

	// Invalidates previous edge references/pointers
	uint NewEdge();

	// Invalidates previous edge references/pointers.
	HalfEdge& NewEdge(uint& id);

	// Invalidates previous edge references/pointers.
	uint NewEdge(const HalfEdge hEdge);

	// Invalidates previous edge references/pointers.
	HalfEdge& NewEdge(const HalfEdge hEdge, uint& id);
	uint FaceEdgeCount(uint faceId);
	VertexAdjacentInfo GetVertexAdjacentInfo(uint vertId);
	cy::Vec3f ApplyCatmullClarkMask(uint vertId, uint edgeCount, uint faceCount);
	cy::Vec3f ApplyLoopMask(uint vertId, uint edgeCount);
	cy::Vec3f ApplyHybridMask(uint vertId, uint edgeCount, uint quadCount);
	cy::Vec3f faceCentroid(uint faceId);
	float approxFaceSurfaceArea(uint faceId);
	cy::Vec3f computeCentroid();
	void translateVertices(const cy::Vec3f& add);
	void build(const std::vector<cy::Vec4<uint>>& faces);

	std::vector<uint> faceEdges; // Each faces' first edge

	std::vector<cy::Vec3f> vertPos; // Each vertices' position
	std::vector<uint> vertEdge; // Each vertices' first edge

	std::vector<HalfEdge> edges; // Each half-edge

public:

	cy::Vec3f scale = cy::Vec3f(1.0f);
	cy::Vec3f pos = cy::Vec3f(0.0f);
	cy::Vec3f eulerRot = cy::Vec3f(0.0f); // Should later use quaternion instead of euler angles

	void getRenderBuffers(std::vector<cy::Vec3f>& pos, std::vector<cy::Vec3f>& norm, std::vector<cy::Vec3<uint>>& id, uint objId);
	cy::Matrix4f getTransform();
	void deform();
	void GetFaceVertices(uint faceId, uint vertices[4]);
	const cy::Vec3f* VertexPositions() const;
	uint VertexCount() const;
	void GetElementBuffer(std::vector<uint>& elementBuffer);
	void GetEdgeElementBuffer(std::vector<uint>& elementBuffer);

	/// <summary>
	///		Splits an edge in half. Original edge's vertices will still reference a valid outgoing 
	///		half-edge (one with the vertex as it's origin). Additionally, adjacent faces
	///		will still reference a half-edge with one of the original edge's vertices as it's origin.
	/// </summary>
	/// <param name="halfEdge">
	///		A half-edge of the edge to be split.
	/// </param>
	void refineEdge(uint hEdgeId);
	void tesselate();
	void subdivide();
	void loadFromInput();
	bool load(const char* obj);
	void load(const FaceList& faceList);
	void refineEdge(uint hEdgeId) {
		uint twinId = edges[hEdgeId].twin;
		uint hEdge2Id = NewEdge(edges[hEdgeId]);
		uint twin2Id = NewEdge(edges[twinId]);

		// Edge references (won't be invalidated)
		HalfEdge& hEdge = edges[hEdgeId];
		HalfEdge& twin = edges[twinId];
		HalfEdge& hEdge2 = edges[hEdge2Id];
		HalfEdge& hTwin2 = edges[twin2Id];

		cy::Vec3 midVertPos = (vertPos[hEdge.orig] + vertPos[hEdge.dest]) / 2.0f;
		uint midVert = NewVertex(midVertPos, hEdge2Id);

		edges[hEdge.next].prev = hEdge2Id;
		hEdge.next = hEdge2Id;
		hEdge.dest = midVert;
		hEdge.twin = twin2Id;
		
		edges[twin.next].prev = twin2Id;
		twin.next = twin2Id;
		twin.dest = midVert;
		twin.twin = hEdge2Id;
		
		hEdge2.prev = hEdgeId;
		hEdge2.orig = midVert;
		hEdge2.twin = twinId;
		
		hTwin2.prev = twinId;
		hTwin2.orig = midVert;
		hTwin2.twin = hEdgeId;
	}

	void tesselate() {
		// Split every edge into two
		uint prevMeshEdgeCount = edges.size();
		for (uint edgeId = 0; edgeId < prevMeshEdgeCount; ++edgeId) {
			if (edges[edgeId].next < prevMeshEdgeCount) { // If edge hasn't already been refined
				refineEdge(edgeId); 
			}
		}

		// Tesselate faces
		uint prevMeshFaceCount = faceEdges.size();
		for (uint faceId = 0; faceId < prevMeshFaceCount; ++faceId) {
			uint faceEdgeId = faceEdges[faceId];
			HalfEdge* faceEdge = &edges[faceEdgeId];

			uint next = faceEdge->next;
			int faceEdgeCount = 1; // Refers to edge count before splitting

			// Iterate over each corner. Each one besides the first will be associated with a new face.
			while (next != faceEdge->prev) {
				HalfEdge& cornerEdge1 = edges[next];
				HalfEdge& cornerEdge2 = edges[cornerEdge1.next];

				uint faceId = NewFace(next);
				cornerEdge1.face = faceId;
				cornerEdge2.face = faceId;

				++faceEdgeCount;
				next = cornerEdge2.next;
			}
			
			if (faceEdgeCount == 3) {
				// Triangle

				// Clip Each Corner
				for (int i = 0; i < 3; ++i) {
					uint newEdgeId;
					HalfEdge& newEdge = NewEdge(newEdgeId);

					faceEdge = &edges[faceEdgeId]; // Update reference since it could be invalidated
					HalfEdge& faceEdgePrev = edges[faceEdge->prev];
					uint nextCornerEdge = edges[faceEdge->next].next;

					newEdge.orig = faceEdge->dest;
					newEdge.dest = faceEdgePrev.orig;
					newEdge.face = faceEdge->face;
					newEdge.next = faceEdge->prev;
					newEdge.prev = faceEdgeId;

					faceEdge->next = newEdgeId;
					faceEdgePrev.prev = newEdgeId;

					faceEdgeId = nextCornerEdge;
					faceEdge = &edges[faceEdgeId];
				}

				uint centerFaceId = faceEdges.size();
				uint centerFaceFirstEdgeId = edges.size();
				uint centerFaceLastEdgeId = centerFaceFirstEdgeId + 2;
				faceEdges.push_back(centerFaceFirstEdgeId);

				for (uint i = centerFaceFirstEdgeId - 3; i < centerFaceFirstEdgeId; ++i) {
					uint twinId = NewEdge();

					HalfEdge& edge = edges[i];
					edge.twin = twinId;

					HalfEdge& twin = edges[twinId];
					twin.twin = i;
					twin.orig = edge.dest;
					twin.dest = edge.orig;
					twin.face = centerFaceId;
					twin.next = WrapInc(twinId, centerFaceLastEdgeId, centerFaceFirstEdgeId);
					twin.prev = WrapDec(twinId, centerFaceLastEdgeId, centerFaceFirstEdgeId);
				}
			} 
			else {
				// Quad
				cy::Vec3f positionSum = vertPos[faceEdge->orig];
				next = faceEdge->next;
				for (uint i = 0; i < 3; ++i) {
					positionSum += vertPos[edges[next].dest];
					next = edges[edges[next].next].next; // Second Next
				}

				uint firstNewEdge = edges.size();
				uint lastNewEdge = firstNewEdge + 7;

				cy::Vec3f newVertexPos = positionSum / 4.0f;
				uint newVertexId = NewVertex(newVertexPos,lastNewEdge);

				next = faceEdgeId;
				for (uint i = 0; i < 4; ++i) {
					uint newEdgeId = NewEdge();
					uint newEdgeTwinId = NewEdge();

					uint edgeId = next;
					HalfEdge& edge = edges[edgeId];

					uint nextEdgeId = edge.next;
					HalfEdge& nextEdge = edges[nextEdgeId];

					next = nextEdge.next;

					edge.next = newEdgeId;
					nextEdge.prev = newEdgeTwinId;

					HalfEdge& newEdge = edges[newEdgeId];
					HalfEdge& newEdgeTwin = edges[newEdgeTwinId];

					newEdge.twin = newEdgeTwinId;
					newEdgeTwin.twin = newEdgeId;

					newEdge.dest = newVertexId;
					newEdge.orig = edge.dest;
					newEdge.face = edge.face;
					newEdge.prev = edgeId;
					newEdge.next = WrapDec(newEdgeId, lastNewEdge, firstNewEdge);
					
					newEdgeTwin.dest = newEdge.orig;
					newEdgeTwin.orig = newEdge.dest;
					newEdgeTwin.face = nextEdge.face;
					newEdgeTwin.next = nextEdgeId;
					newEdgeTwin.prev = WrapInc(newEdgeTwinId, lastNewEdge, firstNewEdge);
				}
			}
		}

		return;
	}

	void subdivide() {
		tesselate();
		deform();
		return;
	}

	void loadFromInput() {
		std::string filePath;
		std::cout << "Path to .obj file: ";
		std::getline(std::cin, filePath);

		while (!load(filePath.data())) {
			std::cout << "Failed to load .obj file.\nRetry: ";
			std::getline(std::cin, filePath);
		}

		std::cout << "Loaded data successfully!" << std::endl;
	}

	bool load(const char* obj) {
		FaceList faceList;
		bool success = faceList.LoadObj(obj);

		if (!success) { return false; }

		vertPos.swap(faceList.Vertices()); // constant time
		build(faceList.Faces());

		return true;
	}

	void load(const FaceList& faceList) {
		vertPos = faceList.Vertices();
		build(faceList.Faces());
	}

	void load(const std::vector<Vec3f>& vertices, const std::vector<Vec4u>& faces) {
		vertPos = vertices;
		build(faces); 
	}

	void build(const std::vector<cy::Vec4<uint>>& faces) {
		vertEdge.clear();
		faceEdges.clear();
		edges.clear();
		vertEdge.insert(vertEdge.end(), vertPos.size(), MAX_UINT);
		faceEdges.reserve(faces.size());

		// Build
		std::unordered_map<cy::Vec2<uint>, uint> edgeIndices;

		for (uint i = 0; i < faces.size(); ++i) {
			const cy::Vec4<uint>& face = faces[i];

			uint firstEdge = edges.size();
			faceEdges.push_back(firstEdge);

			uint vertCount = 4;
			if (face[3] == MAX_UINT) {
				vertCount = 3;
			}

			for (uint j = 0; j < vertCount; ++j) {
				uint next = WrapInc(j, vertCount - 1);
				uint prev = WrapDec(j, vertCount - 1);

				uint edgeId;
				HalfEdge& edge = NewEdge(edgeId);

				cy::Vec2<uint> edgeVertices(face[j], face[next]);
				edgeIndices[edgeVertices] = edgeId;

				edge.orig = edgeVertices[0];
				edge.dest = edgeVertices[1];
				edge.next = firstEdge + next;
				edge.prev = firstEdge + prev;
				edge.face = i;

				vertEdge[edgeVertices[0]] = edgeId;
			}
		}

		// Connect Twins and Build Exterior Twins
		uint interiorEdgeCount = edges.size();

		for (uint i = 0; i < interiorEdgeCount; ++i) {
			HalfEdge& edge = edges[i];
			cy::Vec2<uint> twinVertices(edge.dest, edge.orig);

			if (edgeIndices.count(twinVertices) != 0) {
				edge.twin = edgeIndices[twinVertices];
			}
			else {
				uint twinIndex;
				HalfEdge& twin = NewEdge(twinIndex);

				twin.orig = edge.dest;
				twin.dest = edge.orig;
				twin.twin = i;

				edges[i].twin = twinIndex;
			}
		}

		int i = 0;

		// Connect exterior twins to each other (if any)
		for (uint i = interiorEdgeCount; i < edges.size(); ++i) {
			HalfEdge& edge = edges[i];

			uint nextId = edge.twin;
			HalfEdge* next = &edges[nextId];

			while (next->prev != MAX_UINT) {
				nextId = edges[next->prev].twin;
				next = &edges[nextId];
			}

			edge.next = nextId;
			next->prev = i;
		}

		translateVertices(-computeCentroid());
	}
};

#endif