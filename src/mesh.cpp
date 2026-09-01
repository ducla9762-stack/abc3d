#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <limits>
#include <unordered_map>
#include <fstream>
#include <cstring>
#include <cctype>
#include <cstdint>

#include "cyVector.h"
#include "cyMatrix.h"

#include "helper.h"
#include "mesh.h"

using std::string;
using std::ifstream;
using std::ostream;

// FaceList

void FaceList::MoveTo(const char*& c_str, char delim) {
	int index = 0;
	while (c_str[index] != '\0' && c_str[index] != delim) {
		++index;
	}
	c_str += index;
}

void FaceList::ProcessObjLine(const string& line) {
	//New Code
	if (line.empty()) { return; }

	//Only "v " as a position line. "vn", "vt", "vp" not counted
	//"f " must not match a hypothetical "fo" tag. 
	const char tag = line[0];
	const char sep = line[1];
	const bool tagged = (sep == ' ' || sep == '\t');

	if (tag == 'v' && tagged) {
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
	}

	else if (tag == 'f' && tagged) {
		Face face(MAX_UINT); 
		const char* segStart = line.c_str(); 
		char* segEnd; 

		for (int i = 0; i < 4; ++i) {
			MoveTo(segStart, ' ');
			
			const char* scan = segStart; 
			while (*scan == ' ' || *scan == '\t') { ++scan; }
			if (*scan == '-') { ++skippedFaces; return; } // Negative (relative) indices unsupported

			uint data = std::strtoul(segStart, &segEnd, 10); 
			uint vertexId = data != 0 ? data - 1 : MAX_UINT; 
			face[i] = vertexId; 
			segStart = segEnd; 
		}

		faces.push_back(face); 
	}

	//"o"/"g" open a new group. Record where it starts so SplitObjects can cut here.
	else if ((tag == 'o' || tag == 'g') && tagged) {
		if (objectStarts.empty() || objectStarts.back() != faces.size())
			objectStarts.push_back(static_cast<uint>(faces.size()));
	}
}

void FaceList::PrintFaceComponent(uint x) const {
	if (x == MAX_UINT) { std::cout << "NONE" << " "; }
	else { std::cout << x << " "; }
}

void FaceList::PrintFace(const Face& face) const {
	std::cout << "f ";
	for (int i = 0; i < 4; ++i) {
		PrintFaceComponent(face[i]);
	}
	std::cout << std::endl;
}

const std::vector<Vec3f>& FaceList::Vertices() const {
	return vertices;
}

const std::vector<Vec4<uint>>& FaceList::Faces() const {
	return faces;
}

uint FaceList::SkippedFaces() const {
	return skippedFaces;
}

std::vector<Vec3f>& FaceList::Vertices() { return vertices; }

std::vector<Vec4<uint>>& FaceList::Faces() { return faces; }

void FaceList::PrintContents() const {
	for (const Vertex& vertex : vertices) {
		std::cout << "v " << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
	}

	for (const Face& face : faces) {
		PrintFace(face);
	}
}

/// <summary>
///		Splits this list into one FaceList per "o"/"g" group, remapping each part's
///		face indices so it only carries the vertices its own faces use. A list with
///		no group tags (any .stl, or a flat .obj) comes back as a single part.
/// </summary>
std::vector<FaceList> FaceList::SplitObjects() const {
	// Turn the group markers into [begin, end) face ranges. Faces written before the
	// first marker form their own leading part, and a list with no markers is one
	// part covering every face.
	std::vector<Vec2u> ranges;
	const uint faceCount = static_cast<uint>(faces.size());
	uint prev = 0;

	for (uint start : objectStarts) {
		if (start > prev) { ranges.push_back(Vec2u(prev, start)); }
		prev = start;
	}

	if (faceCount > prev) { ranges.push_back(Vec2u(prev, faceCount)); }
	if (ranges.empty()) { ranges.push_back(Vec2u(0, faceCount)); }

	std::vector<FaceList> parts;
	parts.reserve(ranges.size());

	for (const Vec2u& range : ranges) {
		FaceList part;
		part.faces.reserve(range[1] - range[0]);

		// .obj indices are file-global, so every part needs its own vertex numbering.
		// Out-of-range indices are copied through untouched, leaving build() to reject them.
		std::unordered_map<uint, uint> remap;

		for (uint f = range[0]; f < range[1]; ++f) {
			Face face = faces[f];

			for (int i = 0; i < 4; ++i) {
				const uint vertId = face[i];
				if (vertId == MAX_UINT || vertId >= vertices.size()) { continue; }

				auto it = remap.find(vertId);
				if (it == remap.end()) {
					it = remap.emplace(vertId, static_cast<uint>(part.vertices.size())).first;
					part.vertices.push_back(vertices[vertId]);
				}

				face[i] = it->second;
			}

			part.faces.push_back(face);
		}

		parts.push_back(std::move(part));
	}

	return parts;
}

bool FaceList::LoadObj(const char* obj) {
	ifstream objFile(obj);
	if (!objFile.is_open()) { return false; }

	string line;
	while (std::getline(objFile, line)) {
		if (line.empty()) { continue; }
		ProcessObjLine(line);
	}

	if (objFile.fail() && !objFile.eof()) { return false; }
	return true;
}

/// <summary>
/// This function attempts to load an STL file into a FaceList object
/// </summary>
/// <param name="stl">The file name for the STL file</param>
/// <returns>
/// true: if the function successfully loads the STL file into the FaceList
/// false: if the function fails to load the STL file. This could happen because: 
///	1) Can't open file
/// 2) Invalid file size
/// 3) Neither a binary or ASCII stl file
/// </returns>
bool FaceList::LoadSTL(const char* stl) {
	ifstream stlFile(stl, std::ios::binary | std::ios::ate);
	if (!stlFile) return false;

	std::streamsize fileSize = stlFile.tellg();
	stlFile.seekg(0, std::ios::beg);

	if (fileSize < 84) return false;

	char header[80];
	stlFile.read(header, 80);

	uint64_t numTriangles = 0;
	stlFile.read(reinterpret_cast<char*>(&numTriangles), 4);
	
	const uint64_t expectedSize = 84ull + static_cast<uint64_t>(numTriangles) * 50ull; 
	bool isBinary = (static_cast<uint64_t>(fileSize) == expectedSize);

	// De-duplicate vertices by exact position using bit-cast hashing.
	// STL stores every triangle's vertices independently, so without this
	// every vertex would be unique and the half-edge twin links can't be built.
	struct Vec3fHash {
		// -0.0f and 0.0f compare equal but have different bit patterns, so they get different results. 
		// This hashing collapses both into the same value
		static uint32_t Bits(float f) {
			uint32_t i;
			memcpy(&i, &f, 4);
			return (i == 0x80000000u) ? 0u : i;
		}

		size_t operator()(const Vec3f& v) const {
			const uint32_t xi = Bits(v.x), yi = Bits(v.y), zi = Bits(v.z);
			size_t h = std::hash<uint32_t>{}(xi);
			h ^= std::hash<uint32_t>{}(yi)+0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<uint32_t>{}(zi)+0x9e3779b9u + (h << 6) + (h >> 2);
			return h;
		}
	};
	struct Vec3fEqual {
		bool operator()(const Vec3f& a, const Vec3f& b) const {
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}
	};
	std::unordered_map<Vec3f, uint, Vec3fHash, Vec3fEqual> vertexMap;

	// Returns the index of this position, inserting it only if not already present.
	auto getOrAddVertex = [&](const Vertex& v) -> uint {
		auto it = vertexMap.find(v);
		if (it != vertexMap.end()) return it->second;
		uint id = static_cast<uint>(vertices.size());
		vertices.push_back(v);
		vertexMap[v] = id;
		return id;
		};
	
	const size_t facesBefore = faces.size(); 
	if (isBinary) {
		faces.reserve(faces.size() + numTriangles);

		for (uint32_t i = 0; i < numTriangles; ++i) {
			stlFile.seekg(12, std::ios::cur); // skip stored normal (we recompute it)

			uint triVerts[3];
			for (int v = 0; v < 3; ++v) {
				Vertex vertex(0.0f);
				stlFile.read(reinterpret_cast<char*>(&vertex.x), 4);
				stlFile.read(reinterpret_cast<char*>(&vertex.y), 4);
				stlFile.read(reinterpret_cast<char*>(&vertex.z), 4);
				triVerts[v] = getOrAddVertex(vertex);
			}

			stlFile.seekg(2, std::ios::cur); // skip attribute byte count
			if (!stlFile) return false; 
			
				faces.push_back(Face(triVerts[0], triVerts[1], triVerts[2], MAX_UINT));
		}

		return faces.size() > facesBefore;
	}
	else {
		stlFile.close();
		ifstream asciiFile(stl);
		if (!asciiFile) return false;

		string line;
		uint loopVertCount = 0;
		uint triVerts[3] = { MAX_UINT, MAX_UINT, MAX_UINT };
		
		while (std::getline(asciiFile, line)) {
			size_t start = line.find_first_not_of(" \t");
			if (start == string::npos) continue;
			line = line.substr(start);

			if (line.substr(0, 6) == "vertex") {
				if (loopVertCount >= 3) continue; // guard against malformed faces
				Vertex vertex(0.0f);
				const char* segStart = line.c_str() + 6;
				char* segEnd;
				for (int i = 0; i < 3; ++i) {
					vertex[i] = std::strtof(segStart, &segEnd);
					segStart = segEnd;
				}
				triVerts[loopVertCount] = getOrAddVertex(vertex);
				++loopVertCount;
			}
			else if (line.substr(0, 10) == "outer loop") { // fix: was 9 chars ("outer loo")
				loopVertCount = 0;
			}
			else if (line.substr(0, 7) == "endloop") {
				if (loopVertCount == 3) {
					faces.push_back(Face(triVerts[0], triVerts[1], triVerts[2], MAX_UINT));
				}
			}
		}

		return faces.size() > facesBefore;
	}
}


// HalfEdgeMesh
uint HalfEdgeMesh::NewFace(uint faceEdge) {
	uint face = faceEdges.size();
	faceEdges.push_back(faceEdge);
	return face;
}

uint HalfEdgeMesh::NewVertex(const Vec3f& pos, uint firstEdgeId) {
	uint newVertexId = VertexCount();

	vertPos.push_back(pos);
	vertEdge.push_back(firstEdgeId);

	return newVertexId;
}

// Invalidates previous edge references/pointers
uint HalfEdgeMesh::NewEdge() {
	uint newEdgeId = edges.size();
	edges.emplace_back();
	return newEdgeId;
}

// Invalidates previous edge references/pointers.
HalfEdgeMesh::HalfEdge& HalfEdgeMesh::NewEdge(uint& id) {
	id = edges.size();
	return edges.emplace_back();
}

// Invalidates previous edge references/pointers.
uint HalfEdgeMesh::NewEdge(const HalfEdge hEdge) {
	uint newEdgeId = edges.size();
	edges.push_back(hEdge);
	return newEdgeId;
}

// Invalidates previous edge references/pointers.
HalfEdgeMesh::HalfEdge& HalfEdgeMesh::NewEdge(const HalfEdge hEdge, uint& id) {
	id = edges.size();
	edges.push_back(hEdge);
	return edges[id];
}

uint HalfEdgeMesh::FaceEdgeCount(uint faceId) {
	uint edgeCount = 0;
	uint first = faceEdges[faceId];
	uint next = first;

	do {
		next = edges[next].next;
		++edgeCount;
	} while (next != first);

	return edgeCount;
}

HalfEdgeMesh::VertexAdjacentInfo HalfEdgeMesh::GetVertexAdjacentInfo(uint vertId) {
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

Vec3f HalfEdgeMesh::ApplyCatmullClarkMask(uint vertId, uint edgeCount, uint faceCount) {
	float edgeCountSqrd = edgeCount * edgeCount;
	float e = 1.0f / edgeCountSqrd;
	float d = 2.0f * e;
	float c = (edgeCount - 3.0f) / float(edgeCount);

	Vec3f newPos(0.0f);

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

Vec3f HalfEdgeMesh::ApplyLoopMask(uint vertId, uint edgeCount) {
	constexpr float TWO_PI = 2.0f * Pi<float>();
	float a = (0.375f + 0.25f * std::cos(TWO_PI / float(edgeCount)));
	a = (a * a * 2.0f) - 0.25f;
	float b = (1.0f - a) / float(edgeCount);

	Vec3f newPos(0.0f);
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

Vec3f HalfEdgeMesh::ApplyHybridMask(uint vertId, uint edgeCount, uint quadCount) {
	float halfEdgeCount = float(edgeCount) / 2.0f;
	float quarterQuadCount = float(quadCount) / 4.0f;
	float alpha = 1.0f / (1.0f + halfEdgeCount + quarterQuadCount);
	float beta = alpha / 2.0f;
	float gamma = alpha / 4.0f;

	Vec3f newPos(0.0f);
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

Vec3f HalfEdgeMesh::faceCentroid(uint faceId) {
	Vec3f sum(0.0f);
	float vertexCount = 0.0f;
	uint first = faceEdges[faceId];
	uint next = first;

	do {
		++vertexCount;
		sum += vertPos[edges[next].orig];
		next = edges[next].next;
	} while (next != first);

	return sum / vertexCount;
}

// Old. Doesn't work with n-gons or non-convex quads yet.
float HalfEdgeMesh::approxFaceSurfaceArea(uint faceId) {
	uint faceVertices[4];
	GetFaceVertices(faceId, faceVertices);
	Vec3f vert0 = vertPos[faceVertices[0]];
	Vec3f vert1 = vertPos[faceVertices[1]];
	Vec3f vert2 = vertPos[faceVertices[2]];

	Vec3f seg01 = vert1 - vert0;
	Vec3f seg02 = vert2 - vert0;
	float area = seg01.Cross(seg02).Length();

	if (faceVertices[3] != MAX_UINT) {
		vert1 = vertPos[faceVertices[3]];
		seg01 = vert1 - vert0;
		area += seg01.Cross(seg02).Length();
	}

	return area;
}

// Old. approxFaceSurfaceArea doesn't work with N-gons yet.
Vec3f HalfEdgeMesh::computeCentroid() {
	uint faceCount = faceEdges.size();
	if (faceCount == 0) return Vec3f(0.0f);

	Vec3f weightedSum(0.0f);
	float areaSum = 0.0f;

	for (uint i = 0; i < faceCount; ++i) {
		float area = approxFaceSurfaceArea(i);
		areaSum += area;
		weightedSum += faceCentroid(i) * area;
	}

	return weightedSum / areaSum;
}

Vec3f HalfEdgeMesh::GetVertexNormal(uint vertId) {
	Vec3f pos = vertPos[vertId];
	uint first = vertEdge[vertId];
	uint next = edges[edges[first].twin].next;
	uint last = next;

	Vec3f temp = vertPos[edges[first].dest];
	Vec3f normalSum(0.0f);
	float weightSum = 0.0f;
	bool odd = false;

	do {
		HalfEdge& edge = edges[next];
		Vec3f adjVertPos = vertPos[edge.dest];
		Vec3f seg0 = temp - pos;
		seg0.Normalize();
		Vec3f seg1 = adjVertPos - pos;
		seg1.Normalize();
			
		Vec3f normal = seg1.Cross(seg0);
		std::cout << normal << std::endl;

		float weight = 1.0f - seg0.Dot(seg1);
		normalSum += normal * weight;
		weightSum += weight;
		temp = adjVertPos;
		next = edges[edge.twin].next;
	} while (next != last);

	Vec3f normal = normalSum / weightSum;
	normal.Normalize();

	return normal;
}

void HalfEdgeMesh::TranslateVertex(uint vertId, const Vec3f& translation) {
	vertPos.at(vertId) += translation;
}

Vec3f HalfEdgeMesh::GetVertexPosition(uint vertId) {
	return vertPos[vertId];
}

void HalfEdgeMesh::SetVertexPosition(uint vertId, const Vec3f& pos) {
	vertPos.at(vertId) = pos;
}

void HalfEdgeMesh::translateVertices(const Vec3f& add) {
	for (Vec3f& vert : vertPos) {
		vert += add;
	}
}

void HalfEdgeMesh::TranslateVertexOut(uint vertId, float factor) {
	TranslateVertex(vertId, factor * GetVertexNormal(vertId));
}

bool HalfEdgeMesh::IsTriangle(uint faceId) {
	uint edge = faceEdges[faceId];
	return edges[edge].prev == edges[edges[edge].next].next;
}

bool HalfEdgeMesh::IsNGon(uint faceId) {
	uint edgeId = faceEdges[faceId];
	const HalfEdge& edge = edges[edgeId];
	const HalfEdge& next = edges[edge.next];
	bool isTri = edge.prev == next.next;
	bool isQuad = edge.prev == edges[next.next].next;
	return !(isTri || isQuad);
}

bool isConvex(const Vec2f& vert1, const Vec2f& vert2, const Vec2f& vert3) {
	const float EPSILON = 0.0001f;
	float orientation = (vert1 - vert2).Cross(vert3 - vert2);
	return orientation < -EPSILON;
}

void HalfEdgeMesh::ClipCorner(uint hEdge, uint& innerClipId, uint& outerClipId, uint& cornerFace) {
	if (IsTriangle(edges[hEdge].face)) { return; }

	innerClipId = NewEdge();
	outerClipId = NewEdge();
	cornerFace = NewFace(hEdge);

	HalfEdge& edge = edges[hEdge];
	HalfEdge& prev = edges[edge.prev];

	HalfEdge& innerClip = edges[innerClipId];
	innerClip.orig = edge.dest;
	innerClip.dest = prev.orig;
	innerClip.prev = hEdge;
	innerClip.next = edge.prev;
	innerClip.twin = outerClipId;
	innerClip.face = cornerFace;

	HalfEdge& outerClip = edges[outerClipId];
	outerClip.orig = innerClip.dest;
	outerClip.dest = innerClip.orig;
	outerClip.prev = prev.prev;
	outerClip.next = edge.next;
	outerClip.twin = innerClipId;
	outerClip.face = edge.face;

	edges[edge.next].prev = outerClipId;
	edges[prev.prev].next = outerClipId;

	edge.face = cornerFace;
	edge.next = innerClipId;

	prev.face = cornerFace;
	prev.prev = innerClipId;
}

Vec2f projectToLocalPlane(const Vec3f& n, const Vec3f& p) {
	uint component = n.y < 0.9f ? 1 : 0;
	Vec3f uBasis(0.0f);
	uBasis[component] = 1.0f;

	uBasis = n.Cross(uBasis);
	uBasis.Normalize();
	Vec3f vBasis = n.Cross(uBasis);

	return Vec2f(p.Dot(uBasis), p.Dot(vBasis));
}

// Doesn't yet work with co-linear edges. Fix this later.
void HalfEdgeMesh::GetTriangulation(uint faceId, std::vector<Vec3u>& triangles) {
	if (IsTriangle(faceId)) { return; }

	struct PolygonCorner {
		Vec2f pos;
		uint id;
		PolygonCorner* next;
		PolygonCorner* prev;
	};

	int numVertices = FaceEdgeCount(faceId);
	PolygonCorner* polyCorners = new PolygonCorner[numVertices];
	uint edgeId = faceEdges[faceId];
	Vec3f normal = NewellNormal(faceId);

	for (int i = 0; i < numVertices; ++i) {
		const HalfEdge& edge = edges[edgeId];
		polyCorners[i].id = edge.orig;
		polyCorners[i].pos = projectToLocalPlane(normal, vertPos[edge.orig]);
		polyCorners[i].next = polyCorners + WrapInc(i, numVertices - 1);
		polyCorners[i].prev = polyCorners + WrapDec(i, numVertices - 1);
		edgeId = edge.next;
	}

	PolygonCorner* corner = polyCorners;

	triangles.clear();
	triangles.reserve(numVertices - 2);

	while (numVertices > 2) {
		if (!isConvex(corner->prev->pos, corner->pos, corner->next->pos)) {
			corner = corner->next;
			continue;
		}

		PolygonCorner* next = corner->next->next;
		bool validEar = true;
		while (next != corner->prev && validEar == true) {
			validEar = !(isConvex(corner->prev->pos, corner->pos, next->pos)
				&& isConvex(corner->pos, corner->next->pos, next->pos)
				&& isConvex(corner->next->pos, corner->prev->pos, next->pos));
			next = next->next;
		}

		if (validEar) {
			corner->prev->next = corner->next;
			corner->next->prev = corner->prev;
			--numVertices;

			triangles.emplace_back(corner->prev->id, corner->id, corner->next->id);
		}

		corner = corner->next;
	}

	delete[] polyCorners;
}

// Check for co-linear edges later (doesn't do this yet).
void HalfEdgeMesh::TriangulateFace(uint faceId) {
	if (IsTriangle(faceId)) { return; }

	struct PolygonCorner {
		uint halfEdge;
		Vec2f pos;
		PolygonCorner* next;
		PolygonCorner* prev;
	};

	int numVertices = FaceEdgeCount(faceId);
	PolygonCorner* polyCorners = new PolygonCorner[numVertices];
	uint edgeId = faceEdges[faceId];
	Vec3f normal = NewellNormal(faceId);

	for (int i = 0; i < numVertices; ++i) {
		const HalfEdge& edge = edges[edgeId];
		polyCorners[i].halfEdge = edgeId;
		polyCorners[i].pos = projectToLocalPlane(normal, vertPos[edge.orig]);
		polyCorners[i].next = polyCorners + WrapInc(i, numVertices - 1);
		polyCorners[i].prev = polyCorners + WrapDec(i, numVertices - 1);
		edgeId = edge.next;
	}

	PolygonCorner* corner = polyCorners;

	while (numVertices > 2) {
		if (!isConvex(corner->prev->pos, corner->pos, corner->next->pos)) {
			corner = corner->next;
			continue;
		}

		PolygonCorner* next = corner->next->next;
		bool validEar = true;
		while (next != corner->prev && validEar == true) {
			validEar = !(isConvex(corner->prev->pos, corner->pos, next->pos)
				&& isConvex(corner->pos, corner->next->pos, next->pos)
				&& isConvex(corner->next->pos, corner->prev->pos, next->pos));
			next = next->next;
		}

		if (validEar) {
			uint innerClip, outerClip, cornerFace;
			ClipCorner(corner->halfEdge, innerClip, outerClip, cornerFace);

			corner->prev->halfEdge = outerClip;
			corner->prev->next = corner->next;
			corner->next->prev = corner->prev;

			--numVertices;
		}

		corner = corner->next;
	}

	delete[] polyCorners;
}

void HalfEdgeMesh::TriangulateAllNGons() {
	// Optimize later by making private version of triangulate face that takes pre-allocated memory
	// so we don't allocate memory for every face
	int faceCount = faceEdges.size();
	for (int i = 0; i < faceCount; ++i) {
		if (!IsNGon(i)) { continue; }
		TriangulateFace(i);
	}
}

#define StartEdgeIteration(faceId) \
							uint first = faceEdges[faceId];\
							uint next = first;\
							do {\
								HalfEdge& edge = edges[next];\
								next = edge.next;
#define EndEdgeIteration	} while (next != first);

// https://www.sciencedirect.com/science/chapter/edited-volume/pii/B978008050755250052X
Vec3f HalfEdgeMesh::NewellNormal(uint faceId) {
	if (IsTriangle(faceId)) {
		Vec3f vertices[3];
		GetTriVertices(faceId, vertices);

		return (vertices[1] - vertices[0]).Cross(vertices[2] - vertices[0]);
	}

	Vec3f normal(0.0f);

	StartEdgeIteration(faceId)
		Vec3f orig = vertPos[edge.orig];
		Vec3f dest = vertPos[edge.dest];
		normal.x += (orig.y - dest.y) * (orig.z + dest.z);
		normal.y += (orig.z - dest.z) * (orig.x + dest.x);
		normal.z += (orig.x - dest.x) * (orig.y + dest.y);
	EndEdgeIteration

	return normal;
}

Vec3f HalfEdgeMesh::GetFaceNormal(uint faceId) {
	uint vertices[4];
	GetFaceVertices(faceId, vertices);

	Vec3f vert0 = vertPos[vertices[0]];
	Vec3f vert1 = vertPos[vertices[1]];
	Vec3f vert2 = vertPos[vertices[2]];

	Vec3f normal = (vert1 - vert0).Cross(vert2 - vert0);

	if (vertices[3] != MAX_UINT) {
		Vec3f vert3 = vertPos[vertices[3]];
		Vec3f normal2 = (vert2 - vert0).Cross(vert3 - vert0);
		float area1 = normal.Length();
		float area2 = normal2.Length();
		normal += normal2;
		normal /= (area1 + area2);
	}

	return normal;
}

Vec3f HalfEdgeMesh::GetEdgeNormal(uint edgeId) {
	uint face0 = edges[edgeId].face;
	uint face1 = edges[edges[edgeId].twin].face;

	Vec3f normal(0.0f);
	float count = 0.0f;
	if (face0 != MAX_UINT) {
		normal += GetFaceNormal(face0);
		++count;
	}
	if (face1 != MAX_UINT) {
		normal += GetFaceNormal(face1);
		++count;
	}
	std::cout << "Edge Normal Length: " << normal.Length() << std::endl;
	return normal / count;
}

void HalfEdgeMesh::TranslateFace(uint faceId, const Vec3f& translation) {
	StartEdgeIteration(faceId)
		TranslateVertex(edge.orig, translation);
	EndEdgeIteration
}

void HalfEdgeMesh::TranslateEdge(uint edgeId, const Vec3f& translation) {
	HalfEdge& edge = edges[edgeId];
	TranslateVertex(edge.orig, translation);
	TranslateVertex(edge.dest, translation);
}

Vec3f vectorProj(const Vec3f& v, const Vec3f& u) {
	return (v.Dot(u) / u.LengthSquared()) * u;
}

void HalfEdgeMesh::getRenderBuffers(std::vector<Vec3f>& pos, std::vector<Vec3f>& norm, std::vector<Vec2u>& ids, std::vector<float>& edgeDist, std::vector<Vec3u>& edgeIds, uint objId) const {
	uint faceCount = faceEdges.size();
	uint vertices[4]{ MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT };
	uint edges[4]{ MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT };
	Matrix3f transform = getTransform3x3();
	for (int i = 0; i < faceCount; ++i) {
		GetFaceElements(i, vertices, edges);

		Vec3f vert0 = (transform * vertPos[vertices[0]]) + this->pos;
		Vec3f vert1 = (transform * vertPos[vertices[1]]) + this->pos;
		Vec3f vert2 = (transform * vertPos[vertices[2]]) + this->pos;

		pos.push_back(vert0);
		pos.push_back(vert1);
		pos.push_back(vert2);

		Vec3f seg01 = vert1 - vert0;
		Vec3f seg12 = vert2 - vert1;
		Vec3f seg20 = vert0 - vert2;

		float dist0 = (seg01 - vectorProj(seg01, seg12)).Length();
		float dist1 = (seg12 - vectorProj(seg12, seg20)).Length();
		float dist2 = (seg20 - vectorProj(seg20, seg01)).Length();
 
		edgeDist.push_back(dist0);
		edgeDist.push_back(dist1);
		edgeDist.push_back(dist2);

		Vec3f normal = (seg01).Cross(vert2 - vert0);

		if (vertices[3] != MAX_UINT) {
			Vec3f vert3 = (transform * vertPos[vertices[3]]) + this->pos;

			pos.push_back(vert0);
			pos.push_back(vert2);
			pos.push_back(vert3);

			Vec3f seg02 = -seg20;
			Vec3f seg23 = vert3 - vert2;
			Vec3f seg30 = vert0 - vert3;

			dist0 = (seg02 - vectorProj(seg02, seg23)).Length();
			dist1 = (seg23 - vectorProj(seg23, seg30)).Length();
			dist2 = (seg30 - vectorProj(seg30, seg02)).Length();

			edgeDist.push_back(dist0);
			edgeDist.push_back(dist1);
			edgeDist.push_back(dist2);

			Vec3f normal2 = (vert2 - vert0).Cross(vert3 - vert0);
			float area1 = normal.Length();
			float area2 = normal2.Length();
			normal += normal2;
			normal /= (area1 + area2);

			norm.push_back(normal);
			norm.push_back(normal);
			norm.push_back(normal);
		}

		norm.push_back(normal);
		norm.push_back(normal);
		norm.push_back(normal);

		Vec2u id(objId, i);
		ids.push_back(id);
		ids.push_back(id);
		ids.push_back(id);

		if (vertices[3] != MAX_UINT) {
			ids.push_back(id);
			ids.push_back(id);
			ids.push_back(id);

			Vec3u edgeId(edges[1], MAX_UINT, edges[0]);
			edgeIds.push_back(edgeId);
			edgeIds.push_back(edgeId);
			edgeIds.push_back(edgeId);

			edgeId.Set(edges[2], edges[3], MAX_UINT);
			edgeIds.push_back(edgeId);
			edgeIds.push_back(edgeId);
			edgeIds.push_back(edgeId);
		}
		else {
			Vec3u edgeId(edges[1], edges[2], edges[0]);
			edgeIds.push_back(edgeId);
			edgeIds.push_back(edgeId);
			edgeIds.push_back(edgeId);
		}
	}
}

Matrix4f HalfEdgeMesh::getTransform() const {
	Matrix4f transform;
	transform.SetRotationZYX(degreesToRadians(eulerRot.x), degreesToRadians(eulerRot.y), degreesToRadians(eulerRot.z));
	transform.cell[0] *= scale.x; transform.cell[1] *= scale.x; transform.cell[2] *= scale.x;
	transform.cell[4] *= scale.y; transform.cell[5] *= scale.y; transform.cell[6] *= scale.y;
	transform.cell[8] *= scale.z; transform.cell[9] *= scale.z; transform.cell[10] *= scale.z;
	transform.cell[12] = pos.x; transform.cell[13] = pos.y; transform.cell[14] = pos.z;
	return transform;
}

Matrix3f HalfEdgeMesh::getTransform3x3() const {
	Matrix3f transform;
	transform.SetRotationZYX(degreesToRadians(eulerRot.x), degreesToRadians(eulerRot.y), degreesToRadians(eulerRot.z));
	transform.cell[0] *= scale.x; transform.cell[1] *= scale.x; transform.cell[2] *= scale.x;
	transform.cell[3] *= scale.y; transform.cell[4] *= scale.y; transform.cell[5] *= scale.y;
	transform.cell[6] *= scale.z; transform.cell[7] *= scale.z; transform.cell[8] *= scale.z;
	return transform;
}

Vec3f HalfEdgeMesh::WorldDirToLocalDir(const Vec3f& worldDir) {
	Matrix3f transform = getTransform3x3();
	transform.Invert();

	return transform * worldDir;
}

void HalfEdgeMesh::deform() {
	uint vertCount = VertexCount();
	std::vector<Vec3f> newPositions;
	newPositions.insert(newPositions.begin(), VertexCount(), Vec3f(0.0f));
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

void HalfEdgeMesh::GetFaceVertices(uint faceId, uint vertices[4]) const {
	const HalfEdge* edge = &edges[faceEdges[faceId]];
	for (uint i = 0; i < 3; ++i) {
		vertices[i] = edge->dest;
		edge = &edges[edge->next];
	}
	vertices[3] = edge->dest != vertices[0] ? edge->dest : MAX_UINT;
}

void HalfEdgeMesh::GetTriVertices(uint faceId, uint vertices[3]) const {
	const HalfEdge& edge = edges[faceEdges[faceId]];

	vertices[0] = edge.orig;
	vertices[1] = edge.dest;
	vertices[2] = edges[edge.next].dest;
}

void HalfEdgeMesh::GetTriVertices(uint faceId, Vec3f vertices[3]) const {
	const HalfEdge& edge = edges[faceEdges[faceId]];

	vertices[0] = vertPos[edge.orig];
	vertices[1] = vertPos[edge.dest];
	vertices[2] = vertPos[edges[edge.next].dest];
}

void HalfEdgeMesh::GetFaceElements(uint faceId, uint vertices[4], uint edges[4]) const {
	uint first = faceEdges[faceId];
	uint next = first;
	for (uint i = 0; i < 3; ++i) {
		const HalfEdge& edge = this->edges[next];
		vertices[i] = edge.orig;
		edges[i] = next;
		next = edge.next;
	}
	if (next != first) {
		vertices[3] = this->edges[next].orig;
		edges[3] = next;
	}
	else {
		vertices[3] = MAX_UINT;
		edges[3] = MAX_UINT;
	}
}

const Vec3f* HalfEdgeMesh::VertexPositions() const {
	return vertPos.data();
}

const void HalfEdgeMesh::GetWorldVertexPositions(std::vector<Vec3f>& vertices) const {
	Matrix3f transform = getTransform3x3();
	for (const Vec3f& vertex : vertPos) {
		vertices.push_back((transform * vertex) + pos);
	}
}

uint HalfEdgeMesh::VertexCount() const {
	return vertPos.size();
}

// Old. Doesn't work with N-gons.
void HalfEdgeMesh::GetElementBuffer(std::vector<uint>& elementBuffer) {
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

void HalfEdgeMesh::GetEdgeElementBuffer(std::vector<uint>& elementBuffer) {
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
void HalfEdgeMesh::refineEdge(uint hEdgeId) {
	uint twinId = edges[hEdgeId].twin;
	uint hEdge2Id = NewEdge(edges[hEdgeId]);
	uint twin2Id = NewEdge(edges[twinId]);

	// Edge references (won't be invalidated)
	HalfEdge& hEdge = edges[hEdgeId];
	HalfEdge& twin = edges[twinId];
	HalfEdge& hEdge2 = edges[hEdge2Id];
	HalfEdge& hTwin2 = edges[twin2Id];

	Vec3 midVertPos = (vertPos[hEdge.orig] + vertPos[hEdge.dest]) / 2.0f;
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

void HalfEdgeMesh::tesselate() {
	// Basic limit for demo
	if (faceEdges.size() * 4 > 100000) { return; }

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
		if (IsNGon(faceId)) {
			TriangulateFace(faceId);
			continue;
		}

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
			Vec3f positionSum = vertPos[faceEdge->orig];
			next = faceEdge->next;
			for (uint i = 0; i < 3; ++i) {
				positionSum += vertPos[edges[next].dest];
				next = edges[edges[next].next].next; // Second Next
			}

			uint firstNewEdge = edges.size();
			uint lastNewEdge = firstNewEdge + 7;

			Vec3f newVertexPos = positionSum / 4.0f;
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

void HalfEdgeMesh::extrude() {

}

void HalfEdgeMesh::bevel() {

}

void HalfEdgeMesh::subdivide() {
	// Basic limit for demo
	if (faceEdges.size() * 4 > 100000) { return; }

	tesselate();
	deform();
	return;
}

void HalfEdgeMesh::Clear() {
	faceEdges.resize(0);
	vertPos.resize(0);
	vertEdge.resize(0);
	edges.resize(0);
	pos.Set(0.0f);
	scale.Set(1.0f);
	eulerRot.Set(0.0f);
}

/// <summary>
/// This function tries to load a file format into the STL file
/// </summary>
/// <param name="file">
/// The file to load
/// </param>
/// <returns>
/// An ImportReport. `status` is Ok when a mesh was built, FileError when the file could
/// not be read or is not a supported format, and InvalidTopology when the geometry was
/// rejected (out-of-range index, a vertex repeated within one face, non-manifold, or a
/// face with no area because its vertices are coincident or collinear).
/// On any failure the mesh is left empty. `skippedFaces` reports face lines the parser
/// dropped but recovered from, so an Ok import can still be missing geometry.
/// </returns>
ImportReport HalfEdgeMesh::load(const char* file) {
	FaceList faceList;

	bool success = false; 

	// Dispatch on the file's actual extension. strstr matched ".obj" anywhere in the
	// path, so a .stl inside a directory like "models.obj.d" went to the .obj parser,
	// and it was case sensitive, so "CUBE.STL" matched neither branch.
	if (hasExtension(file, ".obj"))
		success = faceList.LoadObj(file);

	else if (hasExtension(file, ".stl"))
		success = faceList.LoadSTL(file);

	ImportReport report;
	report.skippedFaces = faceList.SkippedFaces();

	if (!success) { report.status = ImportReport::FileError; return report; }

	vertPos.swap(faceList.Vertices()); // constant time
	if (!build(faceList.Faces(), basicVertexCountFunc)) { report.status = ImportReport::InvalidTopology; }

	return report;
}

bool HalfEdgeMesh::load(const FaceList& faceList) {
	vertPos = faceList.Vertices();
	return build(faceList.Faces(), basicVertexCountFunc);
}

ImportReport loadMeshes(const char* file, std::vector<HalfEdgeMesh>& meshes) {
	FaceList faceList;

	bool success = false;

	if (hasExtension(file, ".obj"))
		success = faceList.LoadObj(file);

	else if (hasExtension(file, ".stl"))
		success = faceList.LoadSTL(file);

	ImportReport report;
	report.skippedFaces = faceList.SkippedFaces();

	if (!success) { report.status = ImportReport::FileError; return report; }

	const std::vector<FaceList> parts = faceList.SplitObjects();
	std::vector<HalfEdgeMesh> built;
	built.reserve(parts.size());

	// All or nothing: one rejected part rejects the file, so a failed import never
	// leaves half a model behind.
	for (const FaceList& part : parts) {
		if (part.Faces().empty()) { continue; } // Vertices with no faces, nothing to build

		built.emplace_back();
		if (!built.back().load(part)) { report.status = ImportReport::InvalidTopology; return report; }
	}

	meshes = std::move(built);

	return report;
}

void HalfEdgeMesh::load(const std::vector<Vec3f>& vertices, const std::vector<Vec4u>& faces) {
	vertPos = vertices;
	build(faces, basicVertexCountFunc);
}

/// <summary>
/// True when `path` ends with `ext`, compared without regard to case.
/// </summary>
bool hasExtension(const std::string& path, const char* ext) {
	const size_t extLen = std::strlen(ext); 
	if (path.size() < extLen) return false; 

	for (size_t i = 0; i < extLen; ++i) {
		if (std::tolower(static_cast<unsigned char>(path[path.size() - extLen + i])) !=
			std::tolower(static_cast<unsigned char>(ext[i]))) {
			return false; 
		}
	}

	return true; 
}

/// <summary>
/// Returns `path` with `ext` appended unless it already ends with `ext`
/// </summary>
std::string ensureExtension(const std::string& path, const char* ext) {
	return hasExtension(path, ext) ? path : path + ext; 
}

void HalfEdgeMesh::appendObj(std::ostream& out, uint& vertOffset) const {
	std::vector<Vec3f> worldPos;
	worldPos.reserve(vertPos.size()); 
	GetWorldVertexPositions(worldPos); 

	for (const Vec3f& v : worldPos)
		out << "v " << v.x << " " << v.y << " " << v.z << "\n"; 

	uint faceCount = faceEdges.size(); 
	for (uint faceId = 0; faceId < faceCount; ++faceId) {
		out << "f";

		uint first = faceEdges[faceId]; 
		uint next = first; 

		do {
			out << " " << (vertOffset + edges[next].orig + 1); 
			next = edges[next].next; 
		} while (next != first); 
		
		out << "\n"; 
	}

	vertOffset += static_cast<uint>(worldPos.size()); 
}

/// <summary>
/// This function will save the HalfEdgeMesh as an obj file
/// </summary>
/// <param name="filePath">The file path including the name to save to</param>
/// <returns>
/// true: if the function can sucessfully save the mesh as an obj file
/// false: if the function failed to save the mesh as an obj file
/// </returns>
bool HalfEdgeMesh::exportObj(const char* filePath) const {
	std::ofstream file(ensureExtension(filePath, ".obj")); 
	if (!file.is_open()) return false; 

	uint vertOffset = 0; 
	appendObj(file, vertOffset); 

	return !file.fail();
}

void HalfEdgeMesh::appendStlTriangles(std::vector<Triangle>& tris) const {
	std::vector<Vec3f> worldPos; 
	worldPos.reserve(vertPos.size()); 
	GetWorldVertexPositions(worldPos); 

	uint faceCount = faceEdges.size(); 
	for (uint faceId = 0; faceId < faceCount; ++faceId) {
		uint verts[4];
		GetFaceVertices(faceId, verts); 

		Vec3f v0 = worldPos[verts[0]]; 
		Vec3f v1 = worldPos[verts[1]]; 
		Vec3f v2 = worldPos[verts[2]]; 
		
		tris.push_back({ v0, v1, v2 }); 

		if (verts[3] != MAX_UINT)
			tris.push_back({ v0, v2, worldPos[verts[3]] }); 
	}
}

/// <summary>
/// Writes world-space triangles out as an stl file. `filePath` is used as given.
/// </summary>
/// <param name="filePath">The file path to write to, extension included</param>
/// <param name="tris">The triangles to write, already in world space</param>
/// <param name="isBinary">
/// true: binary stl format (more optimal for space)
/// false: ASCII stl format (more human readable, but much worse for space)
/// </param>
/// <returns>
/// true: if the function can sucessfully write the stl file
/// false: if the function failed to write the stl file
/// </returns>
bool writeStl(const std::string& filePath, const std::vector<Triangle>& tris, bool isBinary) {
	if (isBinary) {
		std::ofstream file(filePath, std::ios::binary);
		if (!file.is_open()) return false;

		char header[80] = {};
		file.write(header, 80);

		uint32_t triCount = static_cast<uint32_t>(tris.size());
		file.write(reinterpret_cast<const char*>(&triCount), 4);

		for (const Triangle& tri : tris) {
			Vec3f normal = (tri.v1 - tri.v0).Cross(tri.v2 - tri.v0);
			normal.Normalize();
			file.write(reinterpret_cast<const char*>(&normal), 12);
			file.write(reinterpret_cast<const char*>(&tri.v0), 12);
			file.write(reinterpret_cast<const char*>(&tri.v1), 12);
			file.write(reinterpret_cast<const char*>(&tri.v2), 12);
			uint16_t attr = 0;
			file.write(reinterpret_cast<const char*>(&attr), 2);
		}
		return !file.fail();
	}
	else {
		std::ofstream file(filePath);
		if (!file.is_open()) return false;

		file << "solid mesh\n";
		for (const Triangle& tri : tris) {
			Vec3f normal = (tri.v1 - tri.v0).Cross(tri.v2 - tri.v0);
			normal.Normalize();
			file << "  facet normal " << normal.x << " " << normal.y << " " << normal.z << "\n";
			file << "    outer loop\n";
			file << "      vertex " << tri.v0.x << " " << tri.v0.y << " " << tri.v0.z << "\n";
			file << "      vertex " << tri.v1.x << " " << tri.v1.y << " " << tri.v1.z << "\n";
			file << "      vertex " << tri.v2.x << " " << tri.v2.y << " " << tri.v2.z << "\n";
			file << "    endloop\n";
			file << "  endfacet\n";
		}
		file << "endsolid mesh\n";
		return !file.fail();
	}
}

/// <summary>
/// This function will save the HalfEdgeMesh as an stl file
/// </summary>
/// <param name="filePath">The file path including the name to save to</param>
/// <param name="isBinary">
/// true: if the user wishes to save in binary stl format (more optimal for space)
/// false: if the user wishes to save in ASCII stl format (more human readable, but much worse for space)
/// </param>
/// <returns>
/// true: if the function can sucessfully save the mesh as an stl file
/// false: if the function failed to save the mesh as an stl file
/// </returns>
bool HalfEdgeMesh::exportStl(const char* filePath, bool isBinary) const {
	std::vector<Triangle> tris; 
	appendStlTriangles(tris); 
	
	return writeStl(ensureExtension(filePath, ".stl"), tris, isBinary); 
}