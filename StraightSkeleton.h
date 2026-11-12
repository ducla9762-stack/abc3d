#if __has_include(<CGAL/straight_skeleton_2.h>)

#ifndef _ABC3D_STRAIGHT_SKELETON_H_
#define _ABC3D_STRAIGHT_SKELETON_H_

#define CGAL_AVAILABLE

#include <iostream>
#include <vector>
#include <unordered_map>

#include <CGAL/point_2.h>
#include <CGAL/straight_skeleton_2.h>
#include <CGAL/create_straight_skeleton_2.h>

#include "helper.h"
#include "mesh.h"

typedef CGAL::Straight_skeleton_2<CGAL::Exact_predicates_inexact_constructions_kernel> StraightSkeleton;

class InsetBuilder {
	HalfEdgeMesh& mesh;

	typedef CGAL::Point_2<CGAL::Epick> Vec2_CGAL;
	typedef std::vector<Vec2_CGAL> Polygon;

	class LoadPolygonFunc {
		Polygon& pgon;
		const Vec3f& u;
		const Vec3f& v;
		Vec3f& first;
	public:
		LoadPolygonFunc(Polygon& pgon, const Vec3f& u, const Vec3f& v, Vec3f& first) : pgon(pgon), u(u), v(v), first(first) {}
		void operator()(const Vec3f& vert) {
			Vec2f proj;
			if (pgon.empty()) { proj.Set(0.0f); first = vert; }
			else { proj = projectToUV(vert - first, u, v); }
			pgon.emplace_back(double(proj.x), double(proj.y));
		}
		const Vec3f& GetFirst() { return first; }
	};

public:
	InsetBuilder(HalfEdgeMesh& mesh) : mesh(mesh) {}

	void FullInset(uint face, std::vector<uint>& newVerts) {
		Polygon polygon;

		Vec3f normal = mesh.GetFaceNormal(face);
		Vec3f u, v;

		planeBases(normal, u, v);
		Vec3f first;
		
		mesh.ForEachVertexInFace(face, LoadPolygonFunc(polygon, u, v, first));

		std::shared_ptr<StraightSkeleton> skel = CGAL::create_interior_straight_skeleton_2(polygon.begin(), polygon.end());

		bool found = false;
		
		auto sEdge = skel->halfedges_begin();
		while (sEdge != skel->halfedges_end()) {
			if (!sEdge->is_bisector() && !sEdge->is_border() && CGAL::compare_xy(polygon[1], sEdge->vertex()->point()) == CGAL::EQUAL) {
				found = true;
				break;
			}
			++sEdge;
		}

		assert(found);
		std::unordered_map<int, uint> vertMap;

		uint edge = mesh.faceEdges[face];
		for (uint i = 0; i < polygon.size(); ++i) {
			sEdge->reset_id(edge);

			vertMap[sEdge->vertex()->id()] = mesh.edges[edge].dest;

			sEdge = sEdge->next()->opposite()->next();
			edge = mesh.edges[edge].next;
		}

		int newEdge = int(mesh.edges.size());
		for (auto it = skel->halfedges_begin(); it != skel->halfedges_end(); ++it) {
			if (it->is_bisector()) { it->reset_id(newEdge); ++newEdge; }
		}

		mesh.edges.insert(mesh.edges.end(), size_t(newEdge) - mesh.edges.size(), HalfEdgeMesh::HalfEdge());

		uint newVert = mesh.vertPos.size();
		for (auto it = skel->vertices_begin(); it != skel->vertices_end(); ++it) {
			if (!it->is_contour()) { 
				vertMap[it->id()] = newVert; 
				newVerts.push_back(newVert);
				++newVert; 
				const Vec2_CGAL& uv = it->point();
				Vec2f pos(float(uv.x()), float(uv.y()));
				mesh.vertPos.push_back(projectFromUV(pos, first, u, v));
				mesh.vertEdge.push_back(it->halfedge()->opposite()->id());
			}
		}

		uint faceOffset = uint(mesh.faceEdges.size());
		mesh.faceEdges.insert(mesh.faceEdges.end(), skel->size_of_faces() - 1, MAX_UINT);
		auto faceIt = skel->faces_begin();

		while (faceIt != skel->faces_end()) {
			if (faceIt->id() == 0) { mesh.faceEdges[face] = faceIt->halfedge()->id(); }
			else { mesh.faceEdges[faceIt->id() + faceOffset - 1] = faceIt->halfedge()->id(); }
			++faceIt;
		}

		for (auto it = skel->halfedges_begin(); it != skel->halfedges_end(); ++it) {
			if (it->is_border()) { continue; }
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[it->id()];
			if (it->is_bisector()) {
				edge.dest = vertMap[it->vertex()->id()];
				edge.orig = vertMap[it->prev()->vertex()->id()];
				edge.twin = it->opposite()->id();
			}
			edge.face = it->face()->id() == 0 ? face : (it->face()->id() + faceOffset - 1);
			edge.next = it->next()->id();
			edge.prev = it->prev()->id();
		}
	}

	void FullInset(const std::vector<uint>& faces, std::vector<uint>& newFaces) {
		
	}
};

#endif
#endif