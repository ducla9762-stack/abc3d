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

using std::string;
using std::ifstream;
using std::ostream;

typedef unsigned uint;

class RenderBufferBuilder {

	std::vector<Vec3f> pos;
	std::vector<Vec3f> norm;
	std::vector<Vec2u> ids;
	std::vector<float> edgeDist;
	std::vector<Vec3u> edgeIds;

	void AddTriangles(	const std::vector<Vec3u>& triVertices, const std::vector<Vec3u>& triEdges, const std::vector<Vec3f>& vertices, 
						const Matrix3f& transform, const Vec3f& normal, const Vec3f& objPos, const Vec2u& ids) 
	{
		for (int i = 0; i < triVertices.size(); ++i) {
			const Vec3u triangle = triVertices[i];

			Vec3f vert[3];
			vert[0] = (transform * vertices[triangle[0]]) + objPos;
			vert[1] = (transform * vertices[triangle[1]]) + objPos;
			vert[2] = (transform * vertices[triangle[2]]) + objPos;

			this->pos.insert(this->pos.end(), vert, vert + 3);

			Vec3f seg[3];
			seg[0] = vert[1] - vert[0];
			seg[1] = vert[2] - vert[1];
			seg[2] = vert[0] - vert[2];

			float dist[3];
			dist[0] = (seg[0] - vectorProj(seg[0], seg[1])).Length();
			dist[1] = (seg[1] - vectorProj(seg[1], seg[2])).Length();
			dist[2] = (seg[2] - vectorProj(seg[2], seg[0])).Length();
			
			this->edgeDist.insert(this->edgeDist.end(), dist, dist + 3);

			this->norm.insert(this->norm.end(), 3, normal);
			this->ids.insert(this->ids.end(), 3, ids);
			this->edgeIds.insert(this->edgeIds.end(), 3, triEdges[i]);
		}
	}

public:

	void AddMesh(const HalfEdgeMesh& mesh, uint objId) {
		uint faceCount = mesh.faceEdges.size();
		Matrix3f transform = mesh.getTransform3x3();

		std::vector<Vec3u> triVertices; triVertices.reserve(50);
		std::vector<Vec3u> triEdges; triEdges.reserve(50);
		Vec2u ids(objId, 0);

		for (uint i = 0; i < faceCount; ++i) {
			triVertices.clear(); // Clear should preserve capacity (allocated memory)
			triEdges.clear();

			//std::cout << "Triangulating Face " << i << std::endl;
			mesh.GetTriangulation(i ,triVertices, triEdges);
			//std::cout << "Triangulated Face " << i << std::endl;
			Vec3f normal = mesh.NewellNormal(i);
			ids[1] = i;

			//std::cout << "Begin Adding Triangles For Face: " << i << std::endl;
			AddTriangles(triVertices, triEdges, mesh.vertPos, transform, normal, mesh.pos, ids);
			//std::cout << "Added Triangles For Face: " << i << std::endl;
		}
	}

	void Clear() {
		// Clear data but keep memory allocated
		pos.clear();
		norm.clear();
		ids.clear();
		edgeDist.clear();
		edgeIds.clear();
	}

	void Deallocate() {
		// Deallocate memory
		std::vector<Vec3f>().swap(pos);
		std::vector<Vec3f>().swap(norm);
		std::vector<Vec2u>().swap(ids);
		std::vector<float>().swap(edgeDist);
		std::vector<Vec3u>().swap(edgeIds);
	}

	const std::vector<Vec3f>& PosBuffer() { return pos; }
	const std::vector<Vec3f>& NormBuffer() { return norm; }
	const std::vector<Vec2u>& IdBuffer() { return ids; }
	const std::vector<float>& EdgeDistBuffer() { return edgeDist; }
	const std::vector<Vec3u>& EdgeIdBuffer() { return edgeIds; }
};

#endif