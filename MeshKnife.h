#ifndef _ABC3D_MESH_KNIFE_H
#define _ABC3D_MESH_KNIFE_H

#include "mesh.h"

/// <summary>
/// Performs knife operations on any given half edge mesh
/// </summary>
class HalfEdgeMeshKnife {
	/// <summary>
	/// Represents the point where the knife line has intersected with the mesh
	/// </summary>
	struct CutPoint {
		uint edgeId;
		float u; //scaling factor of intersection
		Vec3f worldPos;
		bool visible;
	};

	HalfEdgeMesh& mesh; 
	std::vector<CutPoint> cutPoints;
	
	std::vector<CutPoint> FindCutPoints(const Vec2f& a, const Vec2f& b, const Vec3f& camera, bool cutThrough) {
		for (uint e = 0; e < mesh.edges.size(); ++e) {
			const HalfEdgeMesh::HalfEdge& edge = mesh.edges[e];
			uint twin = edge.twin; 
			
			if (twin != MAX_UINT && twin < e) continue; 

			Vec3f c = mesh.LocalPosToWorldPos(mesh.vertPos[edge.orig]); 
			Vec3f d = mesh.WorldPosToLocalPos(mesh.vertPos[edge.dest]); 
		}
	}
};

#endif //ABC3D_KNIFE_TOOL
