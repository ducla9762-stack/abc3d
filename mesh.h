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
#include <unordered_set>

#include "cyVector.h"
#include "cyMatrix.h"

#include "helper.h"

#undef min
#undef max

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

/// <summary>
/// A window for one polygon from vertex indices inside
/// size() and operator[] are the whole interface HalfEdgeMesh::build uses, so build() works on this and Vec4u without changes. 
/// </summary>
struct FaceView {
	const uint* verts = nullptr; 
	uint count = 0; 
	uint operator[](uint i)const { return verts[i]; }
	uint size() const { return count; }
};

//Access to FaceView in build()
inline uint polyVertexCount(const FaceView& face) { return face.count;  }

class FaceList{
	typedef Vec3f Vertex;

	//Polygon storage as one flat vector rather than one vector per face
	//faceVerts holds every face's vertex indices, and faceStarts[i] gives the offset where face i begins
	//Face i will be from [faceStarts[i], faceStarts[i + 1]], so there must be a trailing sentinel (accounted for in the size)
	std::vector<uint> faceVerts; 
	std::vector<uint> faceStarts = {0}; 
	
	std::vector<Vertex> vertices; 
	std::vector<uint> objectStarts;
	uint skippedFaces = 0; // Face lines the parser dropped rather than storing

	void MoveTo(const char*& c_str, char delim);
	void ProcessObjLine(const string& line);
	void PrintFaceComponent(uint x) const;
	void PrintFace(const FaceView& face) const;
	bool isBinarySTL(const std::streamsize fileSize, const uint32_t numTriangles, const char* header) const;

public:
	/// <summary>
	/// An adapter to give a list of polygons. It holds one pointer and synthesizes the container interface. 
	/// </summary>
	struct PolygonList {
		const FaceList* owner; 
		size_t size() const { return owner->FaceCount();  }
		FaceView operator[](uint i)const { return owner->GetFace(i); }
	};

	//size() - 1 because the trailing sentinel
	uint FaceCount() const { return static_cast<uint>(faceStarts.size()) - 1; }
	FaceView GetFace(uint i) const {
		return FaceView{ faceVerts.data() + faceStarts[i], faceStarts[i + 1] - faceStarts[i] };
	};
	PolygonList Polygons() const { return PolygonList{ this }; }

	const std::vector<Vec3f>& Vertices() const;
	std::vector<Vec3f>& Vertices();
	uint SkippedFaces() const;
	void PrintContents() const;
	bool LoadObj(const char* obj);
	/// <summary>
	/// Parses .obj text from an already open stream. LoadObj will use this helper function
	/// This is so that a scene file's geometry uses the exact same line parser
	/// </summary>
	/// <param name="in"></param>
	/// <returns></returns>
	bool LoadObjStream(std::istream& in); 
	bool LoadSTL(const char* stl); //Need to make a specialization of this for max faces to make loading only triangles from STL more memory efficient
	/// <summary>
	///		Splits this list into one FaceList per "o"/"g" group, remapping each part's
	///		face indices so it only carries the vertices its own faces use. A list with
	///		no group tags (any .stl, or a flat .obj) comes back as a single part.
	/// </summary>
	std::vector<FaceList> SplitObjects() const;
};

class HalfEdgeMeshBeveler;

class HalfEdgeMesh {

public:
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

private:

	struct VertexAdjacentInfo {
		uint edgeCount = 0;
		uint quadCount = 0;
		uint triCount = 0;
	};

	friend class RenderBufferBuilder;
	friend class HalfEdgeMeshBeveler;

	/// <summary>
	/// Adds a new face.
	/// </summary>
	/// <param name="faceEdge">Default edge of the face.</param>
	/// <returns>The new face's id</returns>
	uint NewFace(uint faceEdge);

	/// <summary>
	/// Adds a new vertex.
	/// </summary>
	/// <param name="pos">The position of the new vertex.</param>
	/// <param name="firstEdgeId">The id of the vertex's default incident edge</param>
	/// <returns>The new vertex's id</returns>
	uint NewVertex(const Vec3f& pos, uint firstEdgeId);

	/// <summary>
	/// Adds a new empty edge whose members need to be set.
	/// Invalidates prevoius edge references/pointers.
	/// </summary>
	/// <returns>New Edge Id</returns>
	uint NewEdge();

	/// <summary>
	/// Adds a new empty edge whose members need to be set.
	/// Invalidates prevoius edge references/pointers.
	/// </summary>
	/// <param name="id">Will contain new edge's id</param>
	/// <returns>Reference to new edge</returns>
	HalfEdge& NewEdge(uint& id);

	/// <summary>
	/// Adds a new edge.
	/// Invalidates prevoius edge references/pointers.
	/// </summary>
	/// <param name="hEdge">Edge to be copied.</param>
	/// <returns>New Edge Id</returns>
	uint NewEdge(const HalfEdge& hEdge);

	/// <summary>
	///	Adds a new edge.
	/// Invalidates prevoius edge references/pointers.
	/// </summary>
	/// <param name="hEdge">Edge to be copied</param>
	/// <param name="id">Will contain new edge's id</param>
	/// <returns>Reference to new edge</returns>
	HalfEdge& NewEdge(const HalfEdge& hEdge, uint& id);

	void SetTwins(uint edgeId, uint twinId, uint orig, uint dest);
	void InsertPrev(uint edgeId, uint newPrevId);
	uint NewTwin(uint edgeId, uint face);

	/// <param name="faceId">The id of the face whose edges will be counted.</param>
	/// <returns>The number of edges in the face.</returns>
	uint FaceEdgeCount(uint faceId) const;

	/// <param name="vertId">The vertex being queried.</param>
	/// <returns>Adjacent info for the vertex.</returns>
	VertexAdjacentInfo GetVertexAdjacentInfo(uint vertId);

	/// <summary>
	/// Performs the deform stage of subdivision using a subdivision scheme
	/// that hybridizes the Catmull-Clark and Loop schemes.
	/// </summary>
	void deform();

	/// <summary>
	/// Relocates a vertex based on the Catmull-Clark subdivision scheme.
	/// </summary>
	/// <param name="vertId">The vertex to relocate.</param>
	/// <param name="edgeCount">Number of edges incident to the vertex.</param>
	/// <param name="faceCount">Number of faces incident to the vertex.</param>
	/// <returns></returns>
	Vec3f ApplyCatmullClarkMask(uint vertId, uint edgeCount, uint faceCount);

	/// <summary>
	/// Relocates a vertex based on the Loop subdivision scheme.
	/// </summary>
	/// <param name="vertId">The vertex to relocate.</param>
	/// <param name="edgeCount">Number of edges incident to the vertex.</param>
	/// <returns></returns>
	Vec3f ApplyLoopMask(uint vertId, uint edgeCount);

	/// <summary>
	/// Relocates a vertex based on a subdivision scheme that hybridizes
	/// the Catmull-Clark and Loop schemes.
	/// </summary>
	/// <param name="vertId">The vertex to relocate.</param>
	/// <param name="edgeCount">The number of edges incident to the vertex.</param>
	/// <param name="quadCount">The number of quad faces incident to the vertex.</param>
	/// <returns></returns>
	Vec3f ApplyHybridMask(uint vertId, uint edgeCount, uint quadCount);

	/// <summary>
	/// CURRENTLY OUTDATED AND NEEDS UPDATING! Approximates the surface area of a face.
	/// </summary>
	/// <param name="faceId">The face whose area will be approximated.</param>
	/// <returns>Approximate surface area of the face.</returns>
	float approxFaceSurfaceArea(uint faceId);

	/// <summary>
	/// CURRENTLY OUTDATED AND NEEDS UPDATING! Computes the area based centroid of the mesh.
	/// </summary>
	/// <returns>The area based centroid of the mesh.</returns>
	Vec3f computeCentroid();

	/// <summary>
	/// Translate all vertices by a translation factor.
	/// </summary>
	/// <param name="add">Translation factor.</param>
	void translateVertices(const Vec3f& add);

	/// <summary>
	/// Builds the Half-Edge Mesh.
	/// </summary>
	/// <typeparam name="Face">
	/// Representation of a polygon that must implement the index operator
	/// such that indexing by i returns the ith consecutive vertex along the polygonal chain.
	/// </typeparam>
	/// <typeparam name="T"> 
	/// A data structure containing polygonal faces that can be indexed with the index operator. 
	/// </typeparam>
	/// <param name="faces">Structure containing faces of the mesh.</param>
	/// <param name="faceVertexCount">
	/// A function that takes a Face as input and outputs number of vertices in the face.
	/// </param>
	/// <returns>True if the build succeeded. Otherwise, false.</returns>
	template<typename T, typename Face>
	bool build(const T& faces, uint(*faceVertexCount)(const Face&));

	/// <summary>
	/// Computes a single representative normal of a face using Newell's Method.
	/// </summary>
	/// <param name="faceId">A face of the mesh.</param>
	/// <returns>Unit normal direction of the face.</returns>
	Vec3f NewellNormal(uint faceId) const;

	/// <param name="faceId">A face of the mesh.</param>
	/// <returns>True if the face has three vertices. Otherwise, false.</returns>
	bool IsTriangle(uint faceId) const;

	/// <param name="faceId">A face of the mesh.</param>
	/// <returns>True if the face has more than 4 vertices. Otherwise, false.</returns>
	bool IsNGon(uint faceId) const;

	void BevelHalfEdgeCase2(HalfEdge& e1, HalfEdge& e2);
	void BevelHalfEdgeCase1(HalfEdge& e1, HalfEdge& e2);
	/// <summary>Not yet implemented.</summary>
	void BevelEdge(uint edgeId);

	std::vector<uint> faceEdges; // Each faces' first edge

	std::vector<Vec3f> vertPos; // Each vertices' position
	std::vector<uint> vertEdge; // Each vertices' first edge

	std::vector<HalfEdge> edges; // Each half-edge

	Vec3f scale = Vec3f(1.0f);
	Vec3f pos = Vec3f(0.0f);
	Vec3f eulerRot = Vec3f(0.0f); // Should later use quaternion instead of euler angles
public:

	HalfEdge GetEdge(uint edgeId) const {
		return edges.at(edgeId);
	}

	/// <returns>Mesh's scale factor.</returns>
	const Vec3f& Scale() const { return scale; }
	/// <returns>Mesh's world-space position.</returns>
	const Vec3f& Pos() const { return pos; }
	/// <returns>Mesh's rotation represented w/ euler angles.</returns>
	const Vec3f& Rot() const { return eulerRot; }

	/// <param name="scale">New scale factor for the mesh.</param>
	void Scale(const Vec3f& scale) { this->scale = scale; }
	/// <param name="componentId">Component to change.</param>
	/// <param name="component">New component value.</param>
	void Scale(int component, float val) { this->scale[component] = val; }
	/// <param name="pos">New position for the mesh.</param>
	void Pos(const Vec3f& pos) { this->pos = pos; }
	/// <param name="componentId">Component to change.</param>
	/// <param name="component">New component value.</param>
	void Pos(int component, float val) { this->pos[component] = val; }
	/// <param name="eulerRot">New rotation factor for the mesh.</param>
	void Rot(const Vec3f& rot) { this->eulerRot = rot; }
	/// <param name="componentId">Component to change.</param>
	/// <param name="component">New component value.</param>
	void Rot(int component, float val) { this->eulerRot[component] = val; }

	/// <summary> Translates the mesh's world-space position. </summary>
	/// <param name="translation">Factor to translate the position by.</param>
	void TranslatePos(const Vec3f& translation) { pos += translation; }
	/// <summary>Adds to the mesh's scale factor.</summary>
	/// <param name="add">The scale factor to add.</param>
	void AddScale(const Vec3f& add) { this->scale += add; }
	/// <summary>Adds to the mesh's scale factor uniformly.</summary>
	/// <param name="add">The scale factor to add uniformly.</param>
	void AddScale(const float& add) { this->scale += add; }

	/// <returns>Model transformation for the mesh.</returns>
	Matrix4f getTransform() const;
	/// <returns>Scale and rotation components of model transform.</returns>
	Matrix3f getTransform3x3() const;
	/// <param name="worldDir">Direction in world-space.</param>
	/// <returns>The corresponding direction in local-space.</returns>
	Vec3f WorldDirToLocalDir(const Vec3f& worldDir);

	Vec3f LocalPosToWorldPos(const Vec3f& pos) {
		return (getTransform3x3() * pos) + this->pos;
	}

	Vec3f WorldPosToLocalPos(const Vec3f& pos) {
		Matrix3f inv = getTransform3x3();
		inv.Invert();
		return inv * (pos - this->pos);
	}

	/// <param name="faceId">A face of the mesh.</param>
	/// <param name="vertices">
	/// Will contain the first 4 vertices of the face. 
	/// If there are only 3 vertices, vertices[3] = MAX_UINT.
	/// </param>
	void GetQuadVertices(uint faceId, uint vertices[4]) const;

	/// <param name="faceId">A face of the mesh.</param>
	/// <param name="vertices">Will contain the first 3 vertices of the face.</param>
	void GetTriVertices(uint faceId, uint vertices[3]) const;
	/// <param name="faceId">A face of the mesh.</param>
	/// <returns>The first 3 vertices of the face.</returns>
	Vec3u GetTriVertices(uint faceId) const;
	/// <param name="faceId">A face of the mesh.</param>
	/// <param name="vertices">Will contain the positions of the first 3 vertices of the face.</param>
	void GetTriVertices(uint faceId, Vec3f vertices[3]) const;
	/// <param name="faceId">A face of the mesh.</param>
	/// <param name="vertices">A list to push the positions of the first 3 vertices of the face to.</param>
	void GetTriVertices(uint faceId, std::vector<Vec3f>& vertices) const;

	/// <param name="faceId">A face of the mesh.</param>
	/// <returns>The first 3 edges of the face.</returns>
	Vec3u GetTriEdges(uint faceId) const;

	void GetFaceEdges(uint faceId, std::vector<uint>& edges) const;

	/// <param name="faceId">A face of the mesh.</param>
	/// <param name="vertices">
	/// Will contain the first 4 vertices of the face. 
	/// If there are only 3 vertices, vertices[3] = MAX_UINT.
	/// </param>
	/// <param name="edges">
	/// Will contain the first 4 edges of the face. 
	/// If there are only 3 edges, edges[3] = MAX_UINT.</param>
	void GetFaceElements(uint faceId, uint vertices[4], uint edges[4]) const;

	/// <returns>A pointer to the positions of the vertices in the mesh.</returns>
	const Vec3f* VertexPositions() const;

	/// <param name="vertices">A list to push the world-space positions of all vertices in the mesh to.</param>
	const void GetWorldVertexPositions(std::vector<Vec3f>& vertices) const;

	/// <returns>Number of vertices in the mesh.</returns>
	uint VertexCount() const;

	/// <summary>
	/// OUTDATED AND NEEDS UPDATE.
	/// </summary>
	/// <param name="faceId">A face of the mesh.</param>
	/// <returns>Centroid of the face.</returns>
	Vec3f faceCentroid(uint faceId);

	/// <param name="faceId">A face of the mesh.</param>
	/// <param name="triangulation">Triangulation of the face.</param>
	void GetTriangulation(uint faceId, std::vector<Vec3u>& triangulation) const;
	/// <param name="faceId">A face of the mesh.</param>
	/// <param name="triangleVertices">Vertices of the face's triangulation.</param>
	/// <param name="triangleEdges">
	/// Edges of the face's triangulation. An edge will be MAX_UINT if it does not exist in the mesh.
	/// </param>
	void GetTriangulation(uint faceId, std::vector<Vec3u>& triangleVertices, std::vector<Vec3u>& triangleEdges) const;
	/// <param name="faceId">A face to permanently triangulate.</param>
	void TriangulateFace(uint faceId);
	/// <summary>Permanently triangulates all n-gon faces in the mesh.</summary>
	void TriangulateAllNGons();
	/// <summary>Permanently triangulates all quad and n-gon faces in the mesh.</summary>
	void Triangulate();

	/// <summary>
	/// Clips the corner of a face to create a new triangle.
	/// </summary>
	/// <param name="edge">The edge whose origin vertex defines the corner.</param>
	/// <param name="innerClipId">
	/// Will contain the id of the new half-edge that is part of the new triangle. 
	/// </param>
	/// <param name="outerClipId">
	/// Will contain the id of the new half-edge that is exterior to the new triangle.
	/// In other words, it is the id of the new half-edge in the original face.
	/// </param>
	/// <param name="cornerFace">Will contain the id of the new face (triangle).</param>
	void ClipCorner(uint edge, uint& innerClipId, uint& outerClipId, uint& cornerFace);

	/// <param name="vertdId">A vertex of the mesh.</param>
	/// <returns>Position of the vertex.</returns>
	Vec3f GetVertexPosition(uint vertdId);
	/// <param name="vertId">A vertex of the mesh.</param>
	/// <returns>Unit normal direction of the vertex.</returns>
	Vec3f GetVertexNormal(uint vertId);
	/// <param name="faceId">A face of the mesh.</param>
	/// <returns>The unit normal direction of the face.</returns>
	Vec3f GetFaceNormal(uint faceId);
	/// <param name="edgeId">An edge of the mesh.</param>
	/// <returns>The unit normal direction of the edge.</returns>
	Vec3f GetEdgeNormal(uint edgeId);
	/// <returns>The local-space midpoint of an edge.</returns>
	Vec3f GetEdgeCenter(uint edgeId) const;
	/// <summary> Translates a face. </summary>
	/// <param name="faceId">Face to translate.</param>
	/// <param name="translation">Translation factor.</param>
	void TranslateFace(uint faceId, const Vec3f& translation);
	/// <summary> Translates edge. </summary>
	/// <param name="edgeId">Edge to translate.</param>
	/// <param name="translation">Translation factor.</param>
	void TranslateEdge(uint edgeId, const Vec3f& translation);
	/// <summary> Translates vertex. </summary>
	/// <param name="vertId">Vertex to translate.</param>
	/// <param name="translation">Translation factor.</param>
	void TranslateVertex(uint vertId, const Vec3f& translation);
	/// <summary>Translates a vertex along its unit normal direction.</summary>
	/// <param name="vertId">Vertex to translate.</param>
	/// <param name="factor">Ammount to translate as a multiple of the unit normal direction.</param>
	void TranslateVertexOut(uint vertId, float factor);
	void GetOutGoingHalfEdges(uint vertId, std::vector<uint>& halfEdges) const; 
	Vec3f GetHalfEdgeVector(uint hEdgeId) const; 
	uint PickOutgoingHalfEdge(uint vertId, const Vec2f& dragDir, const Vec3f& camRight, const Vec3f& camUp) const; 
	void TranslateVertexAlongEdge(uint vertId, uint hEdgeId, float distance); 
	/// <summary>Directly sets a vertex's position</summary>
	/// <param name="vertId">The vertex to relocate.</param>
	/// <param name="pos">The vertex's new position.</param>
	void SetVertexPosition(uint vertId, const Vec3f& pos);

	uint VertexDegree(uint vertex) {
		uint firstEdge = vertEdge[vertex];
		uint currEdge = firstEdge;
		uint degree = 0;

		do {
			HalfEdgeMesh::HalfEdge& edge = edges[currEdge];
			++degree;
			currEdge = edges[edge.twin].next;
		} while (currEdge != firstEdge);

		return degree;
	}

	/// <summary>
	///		Splits an edge in half. Original edge's vertices will still reference a valid outgoing 
	///		half-edge (one with the vertex as it's origin). Additionally, adjacent faces
	///		will still reference a half-edge with one of the original edge's vertices as it's origin.
	/// </summary>
	/// <param name="halfEdge">
	///		A half-edge of the edge to be split.
	/// </param>
	void refineEdge(uint hEdgeId);
	void flipEdge(uint edgeId) {
		HalfEdge& edge = edges[edgeId];
		HalfEdge& twin = edges[edge.twin];

		if (!(IsTriangle(edge.face) && IsTriangle(twin.face))) { return; }

		HalfEdge* next = &edges[edge.next];
		next->next = edgeId;
		next->prev = twin.prev;

		edge.orig = next->dest;
		twin.dest = next->dest;

		next = &edges[twin.next];
		next->next = edge.twin;
		next->prev = edge.prev;

		edge.dest = next->dest;
		twin.orig = next->dest;

		HalfEdge* prev = &edges[edge.prev];
		prev->next = twin.next;
		prev->prev = edge.twin;
		prev->face = twin.face;

		prev = &edges[twin.prev];
		prev->next = edge.next;
		prev->prev = edgeId;
		prev->face = edge.face;

		edge.prev = edge.next;
		edge.next = twin.prev;

		twin.prev = twin.next;
		twin.next = next->prev;

		faceEdges[edge.face] = edgeId;
		faceEdges[twin.face] = edge.twin;
		vertEdge[prev->orig] = edge.twin;
		vertEdge[next->orig] = edgeId;
	}

	void TwinsShareAdjFace(uint edgeId, bool shareAdjFace[2]) const;
	/// <summary>
	/// Essentially pops a face out from its edge loop, creating new topology that connects
	/// the face to its previous edge loop. Initially, this new topological bridge will have
	/// degenerate geometry (it is made of 0-area quads). Thus, it is recommended to translate 
	/// the extruded face along a direction within the hemisphere defined by the surface normal.
	/// </summary>
	/// <param name="faceId">The face to extrude</param>
	void Extrude(uint faceId);
	/// <summary>Tesselates the mesh. N-gons are triangulated.</summary>
	void tesselate();
	/// <summary>
	/// Subdivides the mesh according to a subdivision scheme that hybridizes
	/// the Catmull-Clark and Loop schemes.
	/// </summary>
	void subdivide();
	/// <summary>Clear mesh data but keep memory allocated.</summary>
	void Clear();
	
	ImportReport load(const char* file);
	bool load(const FaceList& faceList);
	void load(const std::vector<Vec3f>& vertices, const std::vector<Vec4u>& faces);

	template <size_t V, size_t F>
	void load(const std::array<Vec3f, V>& vertices, const std::array<Vec4u, F>& faces);

	bool exportObj(const char* filePath) const;
	bool exportStl(const char* filePath, bool isBinary = true) const;

	/// <summary>
	/// For obj files, we always convert the vertices into world space. 
	/// For scene management, we don't want this to be the case so there is a new parameter.  
	/// </summary>
	/// <param name="out"></param>
	/// <param name="vertOffset"></param>
	/// <param name="worldSpace"></param>
	void appendObj(std::ostream& out, uint& vertOffset, bool worldSpace = true) const;
	/// <summary>
	/// Flatten every face into world-space triangles for .stl export
	/// </summary>
	/// <param name="tris"></param>
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
		
		//Since we're not using Vec4u, we must check for incorrect faces
		if (vertCount < 3) {
			Clear(); 
			return false; 
		}

		for (uint j = 0; j < vertCount; ++j) {
			if (face[j] >= vertLimit) {Clear(); return false; }

			for (uint k = j + 1; k < vertCount; ++k) {
				if (face[j] == face[k]) { Clear(); return false; }
			}
		}

		//Newell accumulation
		//A polygon with no area has no normal, so NewellNormal would divide by 0 (nan)
		//Instead of a single cross product, we do the following: 
		//1) Project the polygon onto each x, y, z 
		//2) Take the signed area of each shadow: YZ -> x component, ZX -> y, XY -> z
		//3) Each term below is the shoelace sum (https://en.wikipedia.org/wiki/Shoelace_formula) with one trapezoid per edge so the total cancels to twice the enclosed signed area. 
		//Every vertex contributes
		//Given an n-gon (pentagon for example) whose 3 corners happen to be collinear, we can still extract the normal
		Vec3<double> newell(0.0); 
		for (uint j = 0; j < vertCount; ++j) {
			const Vec3f& orig = vertPos[face[j]]; 
			const Vec3f& dest = vertPos[face[WrapInc(j, vertCount - 1)]]; 
			newell.x += (orig.y - dest.y) * (orig.z + dest.z); 
			newell.y += (orig.z - dest.z) * (orig.x + dest.x); 
			newell.z += (orig.x - dest.x) * (orig.y + dest.y); 
		}

		if (newell.LengthSquared() == 0.0) {
			Clear(); 
			return false; 
		}
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

// Half-Edge Mesh Beveler

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
		uint patchEdgeCount; // number of edges (and vertices) in this vertex mesh
		uint degree; // number of incident edges of the original vertex
		Vec2u* incidentEdges; // outgoing edge ids and their new origin vertex
		bool* edgeIsBevel; // used to determine whether an outgoing edge is being beveled
		VMesh(uint vertexId) : vertexId(vertexId) { patchEdgeCount = 0; degree = 0; }
		~VMesh() {
			if (incidentEdges) { delete[] incidentEdges; }
			if (edgeIsBevel) { delete[] edgeIsBevel; }
		}
	};

	HalfEdgeMesh& mesh;
	std::vector<uint> vertices;
	std::vector<Vec3f> vertTransforms; // vert positions and translations

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
			juncEdge.prev = firstNewEdge + (2 *  WrapInc(edgeIndex, vMesh.patchEdgeCount - 1));
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

		std::vector<VMesh> vertexMeshes;
		vertexMeshes.reserve(reserveSize);

		// Create vertex meshes
		for (uint edgeId : edges) {
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeId];

			if (edgeMap.count(edgeId) || edgeMap.count(edge.twin)) { continue; }

			edgeMap[edgeId] = 0;
			edgeMap[edge.twin] = 0;

			if (!vertMap.count(edge.orig)) {
				vertMap[edge.orig] = 0;
				vertexMeshes.emplace_back(edge.orig);
			}

			if (!vertMap.count(edge.dest)) {
				vertMap[edge.dest] = 0;
				vertexMeshes.emplace_back(edge.dest);
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
		for (uint hEdge1Id : edges) {
			if (edgeMap[hEdge1Id] == MAX_UINT) { continue; }

			uint hEdge2Id = mesh.edges[hEdge1Id].twin;
			uint side1 = edgeMap[hEdge1Id];
			uint side2 = edgeMap[hEdge2Id];
			uint face = mesh.NewFace(side1);

			uint faceEdges[4] = { mesh.NewTwin(hEdge1Id, face), side1, mesh.NewTwin(hEdge2Id, face), side2 };

			for (uint i = 0; i < 4; ++i) {
				HalfEdgeMesh::HalfEdge& edge = mesh.edges[faceEdges[i]];

				edge.next = faceEdges[WrapInc(i, 3)];
				edge.prev = faceEdges[WrapDec(i, 3)];
			}

			mesh.edges[side1].face = face;
			mesh.edges[side2].face = face;

			edgeMap[hEdge1Id] = MAX_UINT;
			edgeMap[hEdge2Id] = MAX_UINT;
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
			for(uint i = 1; i < degree; ++i) {
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
