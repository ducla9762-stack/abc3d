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

using std::string;
using std::ifstream;
using std::ostream;

typedef unsigned uint;

const uint MAX_UINT = std::numeric_limits<uint>::max();

void combineHash(size_t& base, const size_t& newHash) {
	base = (base << 1) + base + newHash;
}

template<typename T>
struct std::hash<cy::Vec3<T>> {
	size_t operator()(const cy::Vec3<T>& vec) const noexcept {
		std::hash<T> hash;
		size_t val = hash(vec.x);
		combineHash(val, hash(vec.y));
		combineHash(val, hash(vec.z));
		return val;
	}
};

template<typename T>
struct std::hash<cy::Vec2<T>> {
	size_t operator()(const cy::Vec2<T>& vec) const noexcept {
		std::hash<T> hash;
		size_t val = hash(vec.x);
		combineHash(val, hash(vec.y));
		return val;
	}
};

/// <summary> Convert degrees to radians. </summary>
/// <param name="degrees"> An angle in degrees. </param>
/// <returns> An equivalent angle in radians. </returns>
float degreesToRadians(float degrees) {
	return degrees * (cy::Pi<float>() / 180.0f);
}

class FaceList {

	typedef cy::Vec4<uint> Face;
	typedef cy::Vec3f Vertex;

	std::vector<Vertex> vertices;
	std::vector<Face> faces;

	void MoveTo(const char*& c_str, char delim) {
		int index = 0;
		while (c_str[index] != '\0' && c_str[index] != delim) {
			++index;
		}
		c_str += index;
	}

	void ProcessObjLine(const string& line) {
		if (line.empty()) { return; }
		switch (line[0]) {
		case 'v':
		{
			Vertex vertex(0.0f);
			const char* segStart = line.c_str();
			char* segEnd;
			for (int i = 0; i < 3; ++i) {
				MoveTo(segStart, ' ');
				float component = std::strtof(segStart, &segEnd);
				if (segStart == segEnd) { break; }
				vertex[i] = component;
				segStart = segEnd;
			}
			vertices.push_back(vertex);
			break;
		}
		case 'f':
		{
			Face face(MAX_UINT);
			const char* segStart = line.c_str();
			char* segEnd;
			for (int i = 0; i < 4; ++i) {
				MoveTo(segStart, ' ');
				uint data = std::strtoul(segStart, &segEnd, 10);
				uint vertexId = data != 0 ? data - 1 : MAX_UINT;
				face[i] = vertexId;
				segStart = segEnd;
			}
			faces.push_back(face);
			break;
		}
		}
	}

	void PrintFaceComponent(uint x) const {
		if (x == MAX_UINT) { std::cout << "NONE" << " "; }
		else { std::cout << x << " "; }
	}

	void PrintFace(const Face& face) const {
		std::cout << "f ";
		for (int i = 0; i < 4; ++i) {
			PrintFaceComponent(face[i]);
		}
		std::cout << std::endl;
	}

public:

	const std::vector<cy::Vec3f>& Vertices() const {
		return vertices;
	}

	const std::vector<cy::Vec4<uint>>& Faces() const {
		return faces;
	}

	std::vector<cy::Vec3f>& Vertices() { return vertices; }

	std::vector<cy::Vec4<uint>>& Faces() { return faces; }

	void PrintContents() const {
		for (const Vertex& vertex : vertices) {
			std::cout << "v " << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
		}

		for (const Face& face : faces) {
			PrintFace(face);
		}
	}

	bool LoadObj(const char* obj) {
		ifstream objFile(obj);
		if (!objFile.is_open()) { return false; }

		//std::vector<cy::Vec3f> newVertices();
		//std::vector<Face> newFaces;

		string line;
		while (std::getline(objFile, line)) {
			if (line.empty()) { continue; }
			ProcessObjLine(line);
		}

		if (objFile.fail() && !objFile.eof()) { return false; }
		return true;
	}
};

uint WrapDec(uint x, uint wrapTo, uint wrapFrom = 0) {
	if (x == wrapFrom) {
		return wrapTo;
	}
	return x - 1;
}

uint WrapSub(uint x, uint sub, uint wrapTo, uint wrapFrom = 0) {
	uint range = wrapTo - wrapFrom + 1;
	uint offset = (wrapTo - x + sub) % range;
	return wrapTo - offset;
}

uint WrapInc(uint x, uint wrapFrom, uint wrapTo = 0) {
	if (x == wrapFrom) {
		return wrapTo;
	}
	return x + 1;
}

uint WrapAdd(uint x, uint add, uint wrapFrom) {
	return (x + add) % (wrapFrom + 1);
}

uint WrapAdd(uint x, uint add, uint wrapFrom, uint wrapTo) {
	uint range = wrapFrom - wrapTo + 1;
	uint offset = (x + add - wrapTo) % range;
	return wrapTo + offset;
}

class HalfEdgeMesh {

	struct HalfEdge {

		HalfEdge() {}

		HalfEdge(uint orig, uint dest)
			: orig(orig), dest(dest) {
		}

		HalfEdge(uint orig, uint dest, uint face)
			: orig(orig), dest(dest), face(face) {
		}

		uint orig = MAX_UINT;
		uint dest = MAX_UINT;

		uint face = MAX_UINT;

		uint twin = MAX_UINT;
		uint next = MAX_UINT;
		uint prev = MAX_UINT;
	};

	uint NewFace(uint faceEdge) {
		uint face = faceEdges.size();
		faceEdges.push_back(faceEdge);
		return face;
	}

	uint NewVertex(const cy::Vec3f& pos, uint firstEdgeId) {
		uint newVertexId = VertexCount();

		vertPos.push_back(pos);
		vertEdge.push_back(firstEdgeId);

		return newVertexId;
	}

	// Invalidates previous edge references/pointers
	uint NewEdge() {
		uint newEdgeId = edges.size();
		edges.emplace_back();
		return newEdgeId;
	}

	// Invalidates previous edge references/pointers.
	HalfEdge& NewEdge(uint& id) {
		id = edges.size();
		return edges.emplace_back();
	}

	// Invalidates previous edge references/pointers.
	uint NewEdge(const HalfEdge hEdge) {
		uint newEdgeId = edges.size();
		edges.push_back(hEdge);
		return newEdgeId;
	}

	// Invalidates previous edge references/pointers.
	HalfEdge& NewEdge(const HalfEdge hEdge, uint& id) {
		id = edges.size();
		edges.push_back(hEdge);
		return edges[id];
	}

	uint FaceEdgeCount(uint faceId) {
		uint edgeCount = 0;
		uint first = faceEdges[faceId];
		uint next = first;

		do {
			next = edges[next].next;
			++edgeCount;
		} while (next != first);

		return edgeCount;
	}

	struct VertexAdjacentInfo {
		uint edgeCount = 0;
		uint quadCount = 0;
		uint triCount = 0;
	};

	VertexAdjacentInfo GetVertexAdjacentInfo(uint vertId) {
		VertexAdjacentInfo adjInfo;

		uint first = vertEdge[vertId];
		adjInfo.edgeCount = 0;

		uint next = first;

		do {
			HalfEdge& edge = edges[next];
			next = edges[edge.twin].next;
			++adjInfo.edgeCount;
			if (edge.face == MAX_UINT) { continue; }
			if (FaceEdgeCount(edge.face) == 3) { ++adjInfo.triCount; }
			else { ++adjInfo.quadCount; }
		} while (next != first);

		return adjInfo;
	}

	cy::Vec3f ApplyCatmullClarkMask(uint vertId, uint edgeCount, uint faceCount) {
		float edgeCountSqrd = edgeCount * edgeCount;
		float e = 1.0f / edgeCountSqrd;
		float d = 2.0f * e;
		float c = (edgeCount - 3.0f) / float(edgeCount);

		cy::Vec3f newPos(0.0f);

		uint first = vertEdge[vertId];
		uint next = first;

		do {
			HalfEdge& edge = edges[next];
			next = edges[edge.twin].next;

			newPos += vertPos[edge.dest] * d;

			if (edge.face == MAX_UINT) { continue; }

			newPos += vertPos[edges[edge.next].dest] * e;

		} while (next != first);

		newPos += vertPos[vertId] * c;
		return newPos;
	}

	cy::Vec3f ApplyLoopMask(uint vertId, uint edgeCount) {
		constexpr float TWO_PI = 2.0f * cy::Pi<float>();
		float a = (0.375f + 0.25f * std::cos(TWO_PI / float(edgeCount)));
		a = (a * a * 2.0f) - 0.25f;
		float b = (1.0f - a) / float(edgeCount);

		cy::Vec3f newPos(0.0f);
		uint first = vertEdge[vertId];
		uint next = first;

		do {
			HalfEdge& edge = edges[next];
			newPos += vertPos[edge.dest];
			next = edges[edge.twin].next;
		} while (next != first);

		newPos *= b;
		newPos += vertPos[vertId] * a;

		return newPos;
	}

	cy::Vec3f ApplyHybridMask(uint vertId, uint edgeCount, uint quadCount) {
		float halfEdgeCount = float(edgeCount) / 2.0f;
		float quarterQuadCount = float(quadCount) / 4.0f;
		float alpha = 1.0f / (1.0f + halfEdgeCount + quarterQuadCount);
		float beta = alpha / 2.0f;
		float gamma = alpha / 4.0f;

		cy::Vec3f newPos(0.0f);
		uint first = vertEdge[vertId];
		uint next = first;

		do {
			HalfEdge* edge = &edges[next];
			next = edges[edge->twin].next;

			newPos += vertPos[edge->dest] * beta;

			if (edge->face == MAX_UINT) { continue; }

			edge = &edges[edges[edge->next].next];
			bool quad = edge->dest != vertId;

			if (quad) {
				newPos += vertPos[edge->orig] * gamma;
			}

		} while (next != first);

		newPos += vertPos[vertId] * alpha;
		return newPos;
	}

	std::vector<uint> faceEdges; // Each faces' first edge

	std::vector<cy::Vec3f> vertPos; // Each vertices' position
	std::vector<uint> vertEdge; // Each vertices' first edge

	std::vector<HalfEdge> edges; // Each half-edge

public:

	cy::Vec3f scale = cy::Vec3f(1.0f);
	cy::Vec3f pos = cy::Vec3f(0.0f);
	cy::Vec3f eulerRot = cy::Vec3f(0.0f); // Should later use quaternion instead of euler angles

	cy::Matrix4f getTransform() {
		cy::Matrix4f transform;
		transform.SetRotationZYX(degreesToRadians(eulerRot.x), degreesToRadians(eulerRot.y), degreesToRadians(eulerRot.z));
		transform.cell[0] *= scale.x; transform.cell[1] *= scale.x; transform.cell[2] *= scale.x;
		transform.cell[4] *= scale.y; transform.cell[5] *= scale.y; transform.cell[6] *= scale.y;
		transform.cell[8] *= scale.z; transform.cell[9] *= scale.z; transform.cell[10] *= scale.z;
		transform.cell[12] = pos.x; transform.cell[13] = pos.y; transform.cell[14] = pos.z;
		return transform;
	}

	void deform() {
		uint vertCount = VertexCount();
		std::vector<cy::Vec3f> newPositions;
		newPositions.insert(newPositions.begin(), VertexCount(), cy::Vec3f(0.0f));
		for (uint vertId = 0; vertId < vertCount; ++vertId) {
			VertexAdjacentInfo adjInfo = GetVertexAdjacentInfo(vertId);
			if (adjInfo.triCount == 0) {
				newPositions[vertId] = ApplyCatmullClarkMask(vertId, adjInfo.edgeCount, adjInfo.quadCount);
			}
			else if (adjInfo.quadCount == 0) {
				newPositions[vertId] = ApplyLoopMask(vertId, adjInfo.edgeCount);
			}
			else {
				newPositions[vertId] = ApplyHybridMask(vertId, adjInfo.edgeCount, adjInfo.quadCount);
			}
		}
		vertPos.swap(newPositions);

		return;
	}

	void GetFaceVertices(uint faceId, uint vertices[4]) {
		HalfEdge* edge = &edges[faceEdges[faceId]];
		for (uint i = 0; i < 3; ++i) {
			vertices[i] = edge->dest;
			edge = &edges[edge->next];
		}
		vertices[3] = edge->dest != vertices[0] ? edge->dest : MAX_UINT;
	}

	const cy::Vec3f* VertexPositions() const {
		return vertPos.data();
	}

	uint VertexCount() const {
		return vertPos.size();
	}

	void GetElementBuffer(std::vector<uint>& elementBuffer) {
		uint vertices[4]{ MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT };
		uint faceCount = faceEdges.size();
		elementBuffer.reserve(faceCount * 6);
		for (uint i = 0; i < faceCount; ++i) {
			GetFaceVertices(i, vertices);
			elementBuffer.insert(elementBuffer.end(), vertices, vertices + 3);
			if (vertices[3] != MAX_UINT) {
				elementBuffer.push_back(vertices[0]);
				elementBuffer.push_back(vertices[2]);
				elementBuffer.push_back(vertices[3]);
			}
		}
	}

	void GetEdgeElementBuffer(std::vector<uint>& elementBuffer) {
		elementBuffer.reserve(edges.size());
		for (uint edgeId = 0; edgeId < edges.size(); ++edgeId) {
			HalfEdge& edge = edges[edgeId];
			if (edgeId < edge.twin) {
				elementBuffer.push_back(edge.orig);
				elementBuffer.push_back(edge.dest);
			}
		}
	}

	/// <summary>
	///		Splits an edge in half. Original edge's vertices will still reference a valid outgoing 
	///		half-edge (one with the vertex as it's origin). Additionally, adjacent faces
	///		will still reference a half-edge with one of the original edge's vertices as it's origin.
	/// </summary>
	/// <param name="halfEdge">
	///		A half-edge of the edge to be split.
	/// </param>
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
				uint newVertexId = NewVertex(newVertexPos, lastNewEdge);

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
			HalfEdge& next = edges[nextId];

			while (next.prev != MAX_UINT) {
				nextId = edges[next.prev].twin;
				next = edges[nextId];
			}

			edge.next = nextId;
			next.prev = i;
		}
	}
};

#endif