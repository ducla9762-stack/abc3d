#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <limits>
#include <unordered_map>

#include "cyVector.h"
#include "cyMatrix.h"

#include "helper.h"

using std::string;
using std::ifstream;
using std::ostream;

typedef unsigned uint;

//A single world-space triangle for a .stl file
struct Triangle {
	Vec3f v0, v1, v2;
};

/// <summary>
/// The outcome of importing a mesh file. `status` says whether a usable mesh was
/// produced; `skippedFaces` counts face lines the parser dropped but recovered from,
/// so a mesh can import successfully and still be missing geometry.
/// </summary>
struct ImportReport {
	enum Status {
		Ok,              // A mesh was built
		FileError,       // Could not open the file, or it is not a supported .obj/.stl
		InvalidTopology  // Face indices out of range, repeated in one face, non-manifold,
		                 // or a face with no area (coincident or collinear vertices)
	};

	Status status = Ok;
	uint skippedFaces = 0;

	bool ok() const { return status == Ok; }
};

class FaceList{

	typedef Vec4<uint> Face; 
	typedef Vec3f Vertex;

	std::vector<Vertex> vertices;
	std::vector<Face> faces;
	std::vector<uint> objectStarts;
	uint skippedFaces = 0; // Face lines the parser dropped rather than storing

	void MoveTo(const char*& c_str, char delim);
	void ProcessObjLine(const string& line);
	void PrintFaceComponent(uint x) const;
	void PrintFace(const Face& face) const;
	bool isBinarySTL(const std::streamsize fileSize, const uint32_t numTriangles, const char* header) const;

public:

	const std::vector<Vec3f>& Vertices() const;
	const std::vector<Vec4<uint>>& Faces() const;
	std::vector<Vec3f>& Vertices();
	std::vector<Vec4<uint>>& Faces();
	uint SkippedFaces() const;
	void PrintContents() const;
	bool LoadObj(const char* obj);
	bool LoadSTL(const char* stl); //Need to make a specialization of this for max faces to make loading only triangles from STL more memory efficient

	std::vector<FaceList> SplitObjects() const;
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
	uint NewVertex(const Vec3f& pos, uint firstEdgeId);

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
	Vec3f ApplyCatmullClarkMask(uint vertId, uint edgeCount, uint faceCount);
	Vec3f ApplyLoopMask(uint vertId, uint edgeCount);
	Vec3f ApplyHybridMask(uint vertId, uint edgeCount, uint quadCount);
	float approxFaceSurfaceArea(uint faceId);
	Vec3f computeCentroid();
	void translateVertices(const Vec3f& add);
	template<typename T, typename Face>
	bool build(const T& faces, uint(*faceVertexCount)(const Face&));
	Vec3f NewellNormal(uint faceId);

	bool IsTriangle(uint faceId);
	bool IsNGon(uint faceId);

	std::vector<uint> faceEdges; // Each faces' first edge

	std::vector<Vec3f> vertPos; // Each vertices' position
	std::vector<uint> vertEdge; // Each vertices' first edge

	std::vector<HalfEdge> edges; // Each half-edge
public:

	Vec3f scale = Vec3f(1.0f);
	Vec3f pos = Vec3f(0.0f);
	Vec3f eulerRot = Vec3f(0.0f); // Should later use quaternion instead of euler angles

	void getRenderBuffers(std::vector<Vec3f>& pos, std::vector<Vec3f>& norm, std::vector<Vec2u>& ids, std::vector<float>& edgeDist, std::vector<Vec3u>& edgeIds, uint objId) const;
	Matrix4f getTransform() const;
	Matrix3f getTransform3x3() const;
	Vec3f WorldDirToLocalDir(const Vec3f& worldDir);
	void deform();
	void GetFaceVertices(uint faceId, uint vertices[4]) const;
	void GetTriVertices(uint faceId, uint vertices[3]) const;
	void GetTriVertices(uint faceId, Vec3f vertices[3]) const;
	void GetFaceElements(uint faceId, uint vertices[4], uint edges[4]) const;
	const Vec3f* VertexPositions() const;
	const void GetWorldVertexPositions(std::vector<Vec3f>& vertices) const;
	uint VertexCount() const;
	void GetElementBuffer(std::vector<uint>& elementBuffer);
	void GetEdgeElementBuffer(std::vector<uint>& elementBuffer);
	Vec3f faceCentroid(uint faceId);
	void GetTriangulation(uint faceId, std::vector<Vec3u>& triangulation);

	void TriangulateFace(uint faceId);
	void TriangulateAllNGons();
	void ClipCorner(uint edge, uint& innerClipId, uint& outerClipId, uint& cornerFace);

	Vec3f GetVertexPosition(uint vertdId);
	Vec3f GetVertexNormal(uint vertId);
	Vec3f GetFaceNormal(uint vertId);
	Vec3f GetEdgeNormal(uint vertId);
	void TranslateFace(uint faceId, const Vec3f& translation);
	void TranslateEdge(uint edgeId, const Vec3f& translation);
	void TranslateVertex(uint vertId, const Vec3f& translation);
	void TranslateVertexOut(uint vertId, float factor);
	void SetVertexPosition(uint vertId, const Vec3f& pos);

	/// <summary>
	///		Splits an edge in half. Original edge's vertices will still reference a valid outgoing 
	///		half-edge (one with the vertex as it's origin). Additionally, adjacent faces
	///		will still reference a half-edge with one of the original edge's vertices as it's origin.
	/// </summary>
	/// <param name="halfEdge">
	///		A half-edge of the edge to be split.
	/// </param>
	void refineEdge(uint hEdgeId);
	void bevel();
	void extrude();
	void tesselate();
	void subdivide();
	void Clear();
	ImportReport load(const char* file);
	bool load(const FaceList& faceList);
	void load(const std::vector<Vec3f>& vertices, const std::vector<Vec4u>& faces);

	template <size_t V, size_t F>
	void load(const std::array<Vec3f, V>& vertices, const std::array<Vec4u, F>& faces);

	bool exportObj(const char* filePath) const;
	bool exportStl(const char* filePath, bool isBinary = true) const;

	void appendObj(std::ostream& out, uint& vertOffset) const; 
	void appendStlTriangles(std::vector<Triangle>& tris) const; 
};

/// <summary>
///		Imports every object in `file` as its own mesh. An .obj is split on its "o"/"g"
///		groups; an .stl has no object concept, so it always comes back as one mesh.
///		`meshes` is only written when the whole file imports, so a rejected file leaves
///		it untouched.
/// </summary>
ImportReport loadMeshes(const char* file, std::vector<HalfEdgeMesh>& meshes);

/// <summary>
///		True when `path` ends with `ext`, compared without regard to case. The match is
///		anchored to the end of the path, so a directory named "models.obj.d" no longer
///		makes a .stl file inside it look like an .obj.
/// </summary>
bool hasExtension(const std::string& path, const char* ext); 

std::string ensureExtension(const std::string& path, const char* ext); 
bool writeStl(const std::string& filePath, const std::vector<Triangle>& tris, bool isBinary); 

/// <summary>
///		True when the triangle `v0`, `v1`, `v2` has no usable normal, i.e. the points are
///		coincident or collinear. The test is exact rather than tolerance based: it matches
///		the condition under which Normalize() divides by zero, so anything it accepts still
///		exports a finite normal.
/// </summary>
inline bool IsDegenerateFace(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
	return (v1 - v0).Cross(v2 - v0).LengthSquared() == 0.0f;
}

inline uint basicVertexCountFunc(const Vec4u& face) {
	return face[3] == MAX_UINT ? 3 : 4;
}

template<typename T, typename Face>
bool HalfEdgeMesh::build(const T& faces, uint (*faceVertexCount)(const Face&)) {
	vertEdge.resize(0);
	faceEdges.resize(0);
	edges.resize(0);
	vertEdge.insert(vertEdge.end(), vertPos.size(), MAX_UINT);
	faceEdges.reserve(faces.size());

	//Reject out of range indices and degenerate faces
	const uint vertLimit = static_cast<uint>(vertPos.size()); 
	for (uint i = 0; i < faces.size(); ++i) {
		const Face& face = faces[i]; 
		uint vertCount = faceVertexCount(face);

		for (uint j = 0; j < vertCount; ++j) {
			if (face[j] >= vertLimit) {Clear(); return false; }

			for (uint k = j + 1; k < vertCount; ++k) {
				if (face[j] == face[k]) { Clear(); return false; }
			}
		}

		// Distinct indices can still be coincident or collinear, which leaves the face
		// with no direction: the cross product is zero length and normalizing it yields
		// nan. Quads are checked as the same triangle fan the stl writer emits.
		if (IsDegenerateFace(vertPos[face[0]], vertPos[face[1]], vertPos[face[2]])) { Clear(); return false; }

		if (vertCount == 4 &&
			IsDegenerateFace(vertPos[face[0]], vertPos[face[2]], vertPos[face[3]])) { Clear(); return false; }
	}

	//Build
	std::unordered_map<Vec2<uint>, uint> edgeIndices;

	for (uint i = 0; i < faces.size(); ++i) {
		const Face& face = faces[i];

		uint firstEdge = edges.size();
		faceEdges.push_back(firstEdge);

		uint vertCount = faceVertexCount(face);

		for (uint j = 0; j < vertCount; ++j) {
			uint next = WrapInc(j, vertCount - 1);
			uint prev = WrapDec(j, vertCount - 1);

			uint edgeId;
			HalfEdge& edge = NewEdge(edgeId);

			Vec2<uint> edgeVertices(face[j], face[next]);
			if (edgeIndices.count(edgeVertices) != 0) { Clear(); return false; } //Non-manifold
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
		Vec2<uint> twinVertices(edge.dest, edge.orig);

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
	
	//Translate vertices for shapes relative to the centroid instead of centering all shapes to origin
	const Vec3f centroid = computeCentroid();
	translateVertices(-centroid);
	pos += centroid;

	return true;
}

template <size_t V, size_t F>
void HalfEdgeMesh::load(const std::array<Vec3f, V>& vertices, const std::array<Vec4u, F>& faces) {
	vertPos.insert(vertPos.end(), vertices.begin(), vertices.end());
	(void)build(faces, basicVertexCountFunc);
}

#endif