#ifndef _RENDER_BUFFER_BUILDER_H_
#define _RENDER_BUFFER_BUILDER_H_

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <limits>

#include "cyVector.h"
#include "cyMatrix.h"

#include "helper.h"
#include "mesh.h"

#undef max

using std::string;
using std::ifstream;
using std::ostream;

typedef unsigned uint;

struct VertexAttributes {
	Vec3f pos;
	Vec3f norm;
	Vec2u ids;
};

class RenderBufferBuilder {

	std::vector<Vec3f> vertPos;
	std::vector<Vec2u> vertIds;
	std::vector<Vec3f> edges;
	std::vector<Vec2u> edgeIds;

	std::vector<VertexAttributes> attributes;

	void AddTriangles(	const std::vector<Vec3u>& triVertices, const std::vector<Vec3f>& vertices, 
						const Vec3f& normal, const Vec2u& ids) 
	{
		for (int i = 0; i < triVertices.size(); ++i) {
			const Vec3u& tri = triVertices[i];

			for (int j = 0; j < 3; ++i) {
				VertexAttributes& attr = attributes.emplace_back();
				attr.pos = vertices[tri[j]];
				attr.norm = normal;
				attr.ids = ids;
			}
		}
	}

public:

	void AddMeshToAttributes(const HalfEdgeMesh& mesh, uint objId, uint offset) {
		uint faceCount = mesh.faceEdges.size();

		std::vector<Vec3u> triVertices; triVertices.reserve(50);
		std::vector<Vec3u> triEdges; triEdges.reserve(50);
		Vec2u ids(objId, 0);

		for (uint i = 0; i < faceCount; ++i) {
			triVertices.clear(); // Clear should preserve capacity (allocated memory)
			triEdges.clear();

			if (mesh.faceEdges[i] == MAX_UINT) { continue; }

			//std::cout << "Triangulating Face " << i << std::endl;
			mesh.GetTriangulation(i ,triVertices, triEdges);
			//std::cout << "Triangulated Face " << i << std::endl;
			Vec3f normal = mesh.NewellNormal(i);

			//normal = mesh.
			ids[1] = i + offset;

			//std::cout << "Begin Adding Triangles For Face: " << i << std::endl;
			AddTriangles(triVertices, mesh.vertPos, normal, ids);
			//std::cout << "Added Triangles For Face: " << i << std::endl;
		}
	}

	void AddMeshToVertexList(const HalfEdgeMesh& mesh, uint objId, uint offset) {
		vertPos.insert(vertPos.end(), mesh.vertPos.begin(), mesh.vertPos.end());

		uint end = mesh.vertPos.size() + offset;
		for (uint i = offset; i < end + offset; ++i) {
			vertIds.push_back({ objId, i });
		}
	}

	uint AddMeshToEdgeList(const HalfEdgeMesh& mesh, uint objId, uint offset) {
		uint id = 0;
		for (uint i = 0; i < uint(mesh.edges.size()); ++i) {
			const HalfEdgeMesh::HalfEdge& edge = mesh.edges[i];
			if (edge.twin > i && mesh.edges[i].dest != MAX_UINT) {
				edgeIds.push_back({ objId, id + offset });
				edges.push_back(mesh.vertPos[edge.orig]);
				edges.push_back(mesh.vertPos[edge.dest]);
				++id;
			}
		}
		
		return id;
	}

	void Clear() {
		// Clear data but keep memory allocated
		attributes.clear();
	}

	void Deallocate() {
		// Deallocate memory
		std::vector<VertexAttributes>().swap(attributes);
	}

	const std::vector<VertexAttributes>& Attributes() const { return attributes; }
	const std::vector<Vec3f>& VertexPositions() const { return vertPos; }
	const std::vector<Vec2u>& VertexIds() const { return vertIds; }
	const std::vector<Vec3f>& Edges() const { return edges; }
	const std::vector<Vec2u>& EdgeIds() const { return edgeIds; }
};

#endif