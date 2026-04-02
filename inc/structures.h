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
};

#endif