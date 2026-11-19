#ifndef _ABC3D_STRAIGHT_SKELETON_H_
#define _ABC3D_STRAIGHT_SKELETON_H_

#include <iostream>
#include <vector>
#include <unordered_map>

#include "CGAL.h"

#ifdef CGAL_AVAILABLE

#include <CGAL/point_2.h>
#include <CGAL/straight_skeleton_2.h>
#include <CGAL/create_straight_skeleton_2.h>
#include <CGAL/create_offset_polygons_2.h>

#include "helper.h"
#include "mesh.h"


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

	struct InsetInfo {
		std::shared_ptr<StraightSkeleton> skel;
		double tMax;
		Vec3f u, v;
		uint face;
		uint size;
		Vec2_CGAL firstEdgeIndicator;
	};

	std::vector<InsetInfo> faceInfo;
	
	struct InitialMeshInfo {
		uint faces = 0;
		uint edges = 0;
		uint vertices = 0;
	} initialMeshInfo;

public:
	InsetBuilder(HalfEdgeMesh& mesh) : mesh(mesh) {}

	void StartInset(std::vector<uint> faces) {
		Polygon polygon;

		faceInfo.clear();
		faceInfo.reserve(faces.size());

		initialMeshInfo.faces = mesh.faceEdges.size();
		initialMeshInfo.edges = mesh.edges.size();
		initialMeshInfo.vertices = mesh.vertPos.size();

		for (uint face : faces) {
			polygon.clear();
			InsetInfo& info = faceInfo.emplace_back();
			info.face = face;

			// Get normal and basis vectors of plane
			Vec3f normal = mesh.GetFaceNormal(face);
			planeBases(normal, info.u, info.v);

			// Project all points to plane and construct corresponding 2D polygon
			Vec3f first;
			mesh.ForEachVertexInFace(face, LoadPolygonFunc(polygon, info.u, info.v, first));
			info.firstEdgeIndicator = polygon[1];
			info.size = uint(polygon.size());

			// Create the straight skeleton
			info.skel = CGAL::create_interior_straight_skeleton_2(polygon.begin(), polygon.end());

			// Find the time of the last wavefront event
			info.tMax = 0.0;
			for (auto it = info.skel->vertices_begin(); it != info.skel->vertices_end(); ++it) {
				if (it->time() > info.tMax) { info.tMax = it->time(); }
			}
		}
	}

	void PreviewInset(double factor) {
		factor = std::clamp(factor, 0.0, 1.0);

		mesh.edges.resize(initialMeshInfo.edges);
		mesh.vertPos.resize(initialMeshInfo.vertices);
		//mesh.vertEdge.resize(initialMeshInfo.vertices);
		mesh.faceEdges.resize(initialMeshInfo.faces);

		for (const InsetInfo& info : faceInfo) {
			double t = info.tMax * factor;
			std::vector<std::shared_ptr<CGAL::Polygon_2<K>>> polygons = CGAL::create_offset_polygons_2<CGAL::Polygon_2<CGAL::Exact_predicates_inexact_constructions_kernel>>(t, *info.skel);
			for (const std::shared_ptr<CGAL::Polygon_2<K>>& polygon : polygons) {
				uint firstEdge = uint(mesh.edges.size());
				uint firstVert = uint(mesh.vertPos.size());
				//mesh.faceEdges.push_back(firstEdge);
				mesh.edges.insert(mesh.edges.end(), polygon->size(), HalfEdgeMesh::HalfEdge());
				mesh.vertPos.insert(mesh.vertPos.end(), polygon->size(), Vec3f(0.0f));
				uint wrap = uint(polygon->size()) - 1;
				
				for (uint i = 0; i < uint(polygon->size()); ++i) {
					uint relPrev = WrapDec(i, wrap);
					uint relNext = WrapInc(i, wrap);
					uint curr = firstEdge + i;

					// doesn't need face or twin
					HalfEdgeMesh::HalfEdge& edge = mesh.edges[curr];
					edge.prev = firstEdge + relPrev;
					edge.next = firstEdge + relNext;
					edge.orig = firstVert + i;
					edge.dest = firstVert + relNext;

					Vec3f first = mesh.vertPos[mesh.edges[mesh.faceEdges[info.face]].orig];
					mesh.vertPos[edge.orig] = projectFromUV(ToCyVec(polygon->operator[](i)), first, info.u, info.v);
				}
			}
		}
	}

private:

	void EndInset(const InsetInfo& info, double factor) {
		const double T_TOLERANCE = 0.00001f; // not yet used but plan to
		constexpr int MAX_INT = std::numeric_limits<int>::max();

		double t = factor * info.tMax;

		// Find contour edge corresponding to first edge of face ( mesh.faceEdges[face] )
		bool found = false; // for debugging

		auto sEdge = info.skel->halfedges_begin();
		while (sEdge != info.skel->halfedges_end()) {
			if (!sEdge->is_bisector() && !sEdge->is_border() && CGAL::compare_xy(info.firstEdgeIndicator, sEdge->vertex()->point()) == CGAL::EQUAL) {
				found = true;
				break;
			}
			++sEdge;
		}

		assert(found);
		//std::unordered_map<int, uint> vertMap; may not actually need since I found out that you can call an internal id setter

		// Post-process contour edges and vertices
		uint edge = mesh.faceEdges[info.face];
		for (uint i = 0; i < info.size; ++i) {
			sEdge->reset_id(edge);

			//vertMap[sEdge->vertex()->id()] = mesh.edges[edge].dest;
			sEdge->vertex()->reset_id__internal__(mesh.edges[edge].dest);

			sEdge = sEdge->next()->opposite()->next();
			edge = mesh.edges[edge].next;
		}

		// Post-process bisector edges

		int newEdge = int(mesh.edges.size());
		for (auto it = info.skel->halfedges_begin(); it != info.skel->halfedges_end(); ++it) {
			if (!it->is_bisector()) { continue; }

			if (t >= it->prev()->vertex()->time()) { it->reset_id(newEdge); ++newEdge; }
			else { it->reset_id(MAX_INT); }
		}

		// Add new edges to mesh
		mesh.edges.insert(mesh.edges.end(), size_t(newEdge) - mesh.edges.size(), HalfEdgeMesh::HalfEdge());
		Vec3f first = mesh.vertPos[mesh.edges[mesh.faceEdges[info.face]].orig];

		// Post-Process non-contour vertices
		uint newVert = mesh.vertPos.size();
		for (auto it = info.skel->vertices_begin(); it != info.skel->vertices_end(); ++it) {
			if (it->is_contour()) { continue; }

			if (t >= it->time()) {
				//vertMap[it->id()] = newVert; 
				it->reset_id__internal__(newVert);
				//newVerts.push_back(newVert);
				++newVert;
				const Vec2_CGAL& uv = it->point();
				Vec2f pos(float(uv.x()), float(uv.y()));
				mesh.vertPos.push_back(projectFromUV(pos, first, info.u, info.v));
				mesh.vertEdge.push_back(it->halfedge()->opposite()->id());
			}
			else {
				it->reset_id__internal__(MAX_INT);
			}
		}

		// Add faces to mesh to account for those around the central wavefront face
		uint faceOffset = uint(mesh.faceEdges.size());
		mesh.faceEdges.insert(mesh.faceEdges.end(), info.skel->size_of_faces(), MAX_UINT);

		// Post-process face ids in skeleton
		auto faceIt = info.skel->faces_begin();
		while (faceIt != info.skel->faces_end()) {
			mesh.faceEdges[faceIt->id() + faceOffset] = faceIt->halfedge()->id();
			++faceIt;
		}

		std::vector<decltype(info.skel->halfedges_begin())> partialEdges;
		std::vector<uint> newFaceEdges;

		for (auto it = info.skel->halfedges_begin(); it != info.skel->halfedges_end(); ++it) {
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
					Vec2f pt = ((ratio) * ToCyVec(it->vertex()->point()))
						+ ((1.0f - ratio) * ToCyVec(it->prev()->vertex()->point()));

					edge.dest = mesh.NewVertex(projectFromUV(pt, first, info.u, info.v), nextId);
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

			do { sEdge = sEdge->next(); } while (sEdge->vertex()->time() > t);

			if (sEdge->vertex()->time() == t) {
				edge.next = sEdge->next()->id();
				edge.dest = sEdge->vertex()->id();
			}
			else {
				edge.next = sEdge->id();
				edge.dest = mesh.edges[edge.next].orig;
			}

			mesh.edges[edge.next].prev = edgeId;
			mesh.edges[edge.twin].orig = edge.dest;
		}

		for (uint edgeId : newFaceEdges) {
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeId];
			edge.prev = mesh.edges[mesh.edges[mesh.edges[mesh.edges[edge.twin].next].twin].next].twin;
			mesh.edges[edge.prev].next = edgeId;
		}

		bool newFace = false;
		for (uint edgeId : newFaceEdges) {
			HalfEdgeMesh::HalfEdge& edge = mesh.edges[edgeId];
			if (edge.face != MAX_UINT) { continue; }
			uint currFace = info.face;
			if (newFace) { currFace = mesh.NewFace(edgeId); }
			else { mesh.faceEdges[info.face] = edgeId; }
			newFace = true;

			do {
				HalfEdgeMesh::HalfEdge& curr = mesh.edges[edgeId];
				curr.face = currFace;
				edgeId = curr.next;
			} while (mesh.edges[edgeId].face == MAX_UINT);
		}
	}

public:

	void EndInset(float factor) {
		mesh.edges.resize(initialMeshInfo.edges);
		mesh.vertPos.resize(initialMeshInfo.vertices);
		//mesh.vertEdge.resize(initialMeshInfo.vertices);
		mesh.faceEdges.resize(initialMeshInfo.faces);

		factor = std::clamp(factor, 0.0f, 1.0f);

		for (const InsetInfo& info : faceInfo) { EndInset(info, factor); }
	}

	void FullInset(uint face, std::vector<uint>& newVerts, Vec3f& normal) {
		Polygon polygon;

		normal = mesh.GetFaceNormal(face);
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

	void FullInset(const std::vector<uint>& faces, std::vector<uint>& newVerts, std::vector<Vec3f>& normals) {
		for (uint face : faces) {
			Vec3f normal;
			size_t size = newVerts.size();
			FullInset(face, newVerts, normal);
			normals.insert(normals.end(), newVerts.size() - size, normal);
		}
	}
};

#endif
#endif