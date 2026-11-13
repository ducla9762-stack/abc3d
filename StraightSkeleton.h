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
typedef CGAL::Point_2<CGAL::Epick> Vec2_CGAL;

inline Vec2f ToCyVec(const Vec2_CGAL& v) {
	return Vec2f(float(v.x()), float(v.y()));
}

class InsetBuilder {
	HalfEdgeMesh& mesh;
	
	typedef std::vector<Vec2_CGAL> Polygon;
	//typedef CGAL::

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

	void Inset(uint face, float factor) {
		factor = std::clamp(factor, 0.0f, 1.0f);
		double t = 0.0;
		const double T_TOLERANCE = 0.00001f; // not yet used but plan to
		constexpr int MAX_INT = std::numeric_limits<int>::max();

		Polygon polygon;

		Vec3f normal = mesh.GetFaceNormal(face);
		Vec3f u, v;

		planeBases(normal, u, v);
		Vec3f first;

		mesh.ForEachVertexInFace(face, LoadPolygonFunc(polygon, u, v, first));

		std::shared_ptr<StraightSkeleton> skel = CGAL::create_interior_straight_skeleton_2(polygon.begin(), polygon.end());

		for (auto it = skel->vertices_begin(); it != skel->vertices_end(); ++it) {
			if (it->time() > t) { t = it->time(); }
		}

		t = factor * t;

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
		//std::unordered_map<int, uint> vertMap; may not actually need since I found out that you can call an internal id setter

		uint edge = mesh.faceEdges[face];
		for (uint i = 0; i < polygon.size(); ++i) {
			sEdge->reset_id(edge);

			//vertMap[sEdge->vertex()->id()] = mesh.edges[edge].dest;
			sEdge->vertex()->reset_id__internal__(mesh.edges[edge].dest);

			sEdge = sEdge->next()->opposite()->next();
			edge = mesh.edges[edge].next;
		}

		int newEdge = int(mesh.edges.size());
		for (auto it = skel->halfedges_begin(); it != skel->halfedges_end(); ++it) {
			if (!it->is_bisector()) { continue; }

			if (t >= it->prev()->vertex()->time()) { it->reset_id(newEdge); ++newEdge; }
			else { it->reset_id(MAX_INT); }
		}

		mesh.edges.insert(mesh.edges.end(), size_t(newEdge) - mesh.edges.size(), HalfEdgeMesh::HalfEdge());

		uint newVert = mesh.vertPos.size();
		for (auto it = skel->vertices_begin(); it != skel->vertices_end(); ++it) {
			if (it->is_contour()) { continue; }

			if (t >= it->time()) {
				//vertMap[it->id()] = newVert; 
				it->reset_id__internal__(newVert);
				//newVerts.push_back(newVert);
				++newVert;
				const Vec2_CGAL& uv = it->point();
				Vec2f pos(float(uv.x()), float(uv.y()));
				mesh.vertPos.push_back(projectFromUV(pos, first, u, v));
				mesh.vertEdge.push_back(it->halfedge()->opposite()->id());
			}
			else {
				it->reset_id__internal__(MAX_INT);
			}
		}

		uint faceOffset = uint(mesh.faceEdges.size());
		mesh.faceEdges.insert(mesh.faceEdges.end(), skel->size_of_faces(), MAX_UINT);
		auto faceIt = skel->faces_begin();

		while (faceIt != skel->faces_end()) {
			mesh.faceEdges[faceIt->id() + faceOffset] = faceIt->halfedge()->id();
			++faceIt;
		}

		std::vector<decltype(skel->halfedges_begin())> partialEdges;
		std::vector<uint> newFaceEdges;

		for (auto it = skel->halfedges_begin(); it != skel->halfedges_end(); ++it) {
			if (it->is_border() || it->id() >= newEdge) { continue; }
			bool partial = it->vertex()->id() >= newEdge;

			if (partial) {
				partialEdges.push_back(it);

				double start = it->prev()->vertex()->time();
				bool split = start != t;
				
				uint twinId = mesh.NewEdge();
				uint nextId, nextTwinId;

				if (split) {
					nextId = mesh.NewEdge();
					nextTwinId = mesh.NewEdge();
				}

				HalfEdgeMesh::HalfEdge& edge = mesh.edges[it->id()];

				edge.twin = twinId;
				edge.face = it->face()->id();
				edge.prev = it->prev()->id();
				edge.orig = it->prev()->vertex()->id();

				HalfEdgeMesh::HalfEdge& twin = mesh.edges[twinId];

				if (split) {
					double diff = it->vertex()->time() - start;
					double ratio = (t - start) / diff;
					Vec2f pt = ((1.0f - ratio) * ToCyVec(it->vertex()->point()))
						+ (ratio * ToCyVec(it->prev()->vertex()->point()));

					edge.dest = mesh.NewVertex(projectFromUV(pt, first, u, v), nextId);
					edge.next = nextId;
					
					twin.face = it->opposite()->face()->id();
					twin.next = it->opposite()->next()->id();
					twin.orig = edge.dest;
					twin.dest = edge.orig;
					twin.twin = it->id();

					mesh.edges[twin.next].prev = twinId;
					it->opposite()->reset_id(twinId);

					HalfEdgeMesh::HalfEdge& next = mesh.edges[nextId];
					next.prev = it->id();
					next.orig = edge.dest;
					next.face = edge.face;
					next.twin = nextTwinId;

					HalfEdgeMesh::HalfEdge& nextTwin = mesh.edges[nextTwinId];
					nextTwin.dest = next.orig;
					nextTwin.twin = nextId;
					newFaceEdges.push_back(nextTwinId);
				}
				else {
					twin.dest = edge.orig;
					twin.twin = it->id();
					newFaceEdges.push_back(twinId);
				}

				continue;
			}

			HalfEdgeMesh::HalfEdge& edge = mesh.edges[it->id()];
			if (it->is_bisector()) {
				//edge.dest = vertMap[it->vertex()->id()];
				edge.dest = it->vertex()->id();
				//edge.orig = vertMap[it->prev()->vertex()->id()];
				edge.orig = it->prev()->vertex()->id();
				uint debug = it->id();
				float debug1 = it->prev()->vertex()->time();
				edge.twin = it->opposite()->id();
			}
			edge.face = it->face()->id() == it->face()->id() + faceOffset;
			edge.next = it->next()->id();
			edge.prev = it->prev()->id();
		}

		for (auto& sEdge : partialEdges) {
			bool split = mesh.edges[sEdge->id()].dest != MAX_UINT;
			uint edgeId = split ? mesh.edges[sEdge->id()].next : sEdge->id();
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeId];

			do { sEdge = sEdge->next(); } 
			while (sEdge->vertex()->time() > t);

			if (sEdge->vertex()->time() == t) {
				edge.next = sEdge->next()->id();
				edge.dest = sEdge->vertex()->id();
			}
			else {
				edge.next = sEdge->id();
				edge.dest = mesh.edges[edge.next].orig;
			}

			mesh.edges[edge.twin].orig = edge.dest;
			mesh.edges[edge.next].prev = edgeId;
			mesh.edges[edge.twin].orig = edge.dest;
		}

		for (uint edgeId : newFaceEdges) {
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeId];
			edge.next = mesh.edges[mesh.edges[mesh.edges[mesh.edges[edge.twin].next].twin].next].twin;
			mesh.edges[edge.next].prev = edgeId;
		}

		bool newFace = false;
		for (uint edgeId : newFaceEdges) {
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeId];
			if (edge.face != MAX_UINT) { continue; }
			uint currFace = face;
			if (newFace) { currFace = mesh.NewFace(edgeId); }
			else { mesh.faceEdges[face] = edgeId; }
			newFace = true;
			
			do {
				HalfEdgeMesh::HalfEdge& curr = mesh.edges[edgeId];
				curr.face = currFace;
				edgeId = curr.next;
			} while (mesh.edges[edgeId].face == MAX_UINT);
		}
	}

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
		//std::unordered_map<int, uint> vertMap; may not actually need since I found out that you can call an internal id setter

		uint edge = mesh.faceEdges[face];
		for (uint i = 0; i < polygon.size(); ++i) {
			sEdge->reset_id(edge);

			//vertMap[sEdge->vertex()->id()] = mesh.edges[edge].dest;
			sEdge->vertex()->reset_id__internal__(mesh.edges[edge].dest);

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
				//vertMap[it->id()] = newVert; 
				it->reset_id__internal__(newVert);
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
				//edge.dest = vertMap[it->vertex()->id()];
				edge.dest = it->vertex()->id();
				//edge.orig = vertMap[it->prev()->vertex()->id()];
				edge.orig = it->prev()->vertex()->id();
				edge.twin = it->opposite()->id();
			}
			edge.face = it->face()->id() == 0 ? face : (it->face()->id() + faceOffset - 1);
			edge.next = it->next()->id();
			edge.prev = it->prev()->id();
		}
	}

	void FullInset(const std::vector<uint>& faces, std::vector<uint>& newVerts) {
		
	}
};

#endif
#endif