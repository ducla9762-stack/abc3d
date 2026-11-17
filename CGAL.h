#if __has_include(<CGAL/straight_skeleton_2.h>)

#ifndef _ABC3D_CGAL_FEATURES_H_
#define _ABC3D_CGAL_FEATURES_H_

#define CGAL_AVAILABLE

#include <CGAL/point_2.h>
#include <CGAL/straight_skeleton_2.h>
#include <CGAL/create_straight_skeleton_2.h>
#include <CGAL/create_offset_polygons_2.h>

#include "helper.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Straight_skeleton_2<K> StraightSkeleton;
typedef CGAL::Point_2<K> Vec2_CGAL;

inline Vec2f ToCyVec(const Vec2_CGAL& v) {
	return Vec2f(float(v.x()), float(v.y()));
}

class LoadPolygonFunc {
	std::vector<Vec2_CGAL>& pgon;
	const Vec3f& u;
	const Vec3f& v;
	Vec3f& first;
public:
	LoadPolygonFunc(std::vector<Vec2_CGAL>& pgon, const Vec3f& u, const Vec3f& v, Vec3f& first) : pgon(pgon), u(u), v(v), first(first) {}
	void operator()(const Vec3f& vert) {
		Vec2f proj;
		if (pgon.empty()) { proj.Set(0.0f); first = vert; }
		else { proj = projectToUV(vert - first, u, v); }
		pgon.emplace_back(double(proj.x), double(proj.y));
	}
	const Vec3f& GetFirst() { return first; }
};

#endif
#endif
