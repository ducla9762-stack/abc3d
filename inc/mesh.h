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

class HalfEdgeMesh;

class HalfEdgeMeshBeveler {
	HalfEdgeMesh& mesh;
	uint elements[2];
	Vec3f* buf;
public:
	HalfEdgeMeshBeveler(HalfEdgeMesh& mesh) : mesh(mesh) { }
	~HalfEdgeMeshBeveler() { if (buf) delete[] buf; }
	void StartEdgeBevel(uint edgeId);
	void AdjustBevel(float bevelFactor);

	void EndBevel() { }
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

	/// <summary>
	///		Splits an edge in half. Original edge's vertices will still reference a valid outgoing 
	///		half-edge (one with the vertex as it's origin). Additionally, adjacent faces
	///		will still reference a half-edge with one of the original edge's vertices as it's origin.
	/// </summary>
	/// <param name="halfEdge">
	///		A half-edge of the edge to be split.
	/// </param>
	void refineEdge(uint hEdgeId);
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

#endif
