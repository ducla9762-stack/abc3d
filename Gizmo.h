#ifndef _ABC3D_GIZMO_H_
#define _ABC3D_GIZMO_H_

#include <vector>
#include "helper.h"

constexpr float HALF_SQR_2 = 0.7071068f;

/// <summary>
/// Specifications for how the gizmo should draw.
/// </summary>
struct GizmoInstanceSettings {
	Vec3f base; // The point at which the gizmo draws. The start of the arrow, etc.
	Vec3f dir; // The direction the gizmo points.
	float length; // The length of the gizmo shaft.
	float radius; // The radius of the gizmo shaft.
	float tipSize; // The size of the tip of the gizmo (height of arrow, size of cube, etc.)
	Vec3f color; // The color the gizmo.
	Vec2u ids; // The IDs of the gizmo.
};

static const std::vector<GizmoInstanceSettings> DEFAULT_ARROW_SETTINGS{ {
	{
		Vec3f(0.0f, 0.0f, 0.0f), // base
		Vec3f(1.0f, 0.0f, 0.0f), // dir
		0.5f,                    // length
		0.025f,                  // radius
		0.25f,                   // tipSize
		Vec3f(0.0f, 1.0f, 0.0f), // color
		Vec2u(0, 0),             // ids
	},
	{
		Vec3f(0.0f, 0.0f, 0.0f),
		Vec3f(0.0f, 1.0f, 0.0f),
		0.5f,
		0.025f,
		0.25f,
		Vec3f(0.0f, 0.0f, 1.0f),
		Vec2u(0, 1),
	},
	{
		Vec3f(0.0f, 0.0f, 0.0f),
		Vec3f(0.0f, 0.0f, 1.0f),
		0.5f,
		0.025f,
		0.25f,
		Vec3f(1.0f, 0.0f, 0.0f),
		Vec2u(0, 2),
	},
} };

static const std::vector<GizmoInstanceSettings> DEFAULT_MACE_SETTINGS = { {
	{
		Vec3f(0.0f, 0.0f, 0.0f), // base
		Vec3f(1.0f, 0.0f, 0.0f), // dir
		0.5f,                    // length
		0.025f,                  // radius
		0.1f,                    // tipSize
		Vec3f(0.0f, 1.0f, 0.0f), // color
		Vec2u(1, 0),             // ids
	},
	{
		Vec3f(0.0f, 0.0f, 0.0f),
		Vec3f(0.0f, 1.0f, 0.0f),
		0.5f,
		0.025f,
		0.1f,
		Vec3f(0.0f, 0.0f, 1.0f),
		Vec2u(1, 1),
	},
	{
		Vec3f(0.0f, 0.0f, 0.0f),
		Vec3f(0.0f, 0.0f, 1.0f),
		0.5f,
		0.025f,
		0.1f,
		Vec3f(1.0f, 0.0f, 0.0f),
		Vec2u(1, 2),
	},
} };

const std::vector<Vec3f> ARROW_VERTICES = {
	// Cylinder Base
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
	// Cylinder Top
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
	// Pyramid Base
	Vec3f(2.0, 0.0f, -2.0f), Vec3f(-2.0f, 0.0f, -2.0f), Vec3f(-2.0f, 0.0f, 2.0f),
	Vec3f(2.0f, 0.0f, 2.0f),
	// Pyramid Top
	Vec3f(0.0f, 0.0f, 0.0f)
};

const std::vector<Vec3u> ARROW_TRIANGLES = {
	// Clinder Base
	Vec3u(0, 1, 2), Vec3u(0, 2, 3), Vec3u(0, 3, 4), Vec3u(0, 4, 5), Vec3u(0, 5, 6), Vec3u(0, 6, 7),
	// Cyclinder Top
	Vec3u(8, 10, 9), Vec3u(8, 11, 10), Vec3u(8, 12, 11), Vec3u(8, 13, 12), Vec3u(8, 14, 13), Vec3u(8, 15, 14),
	// Cylinder Sides
	Vec3u(0, 8, 1), Vec3u(1, 8, 9),
	Vec3u(1, 9, 2), Vec3u(2, 9, 10),
	Vec3u(2, 10, 3), Vec3u(3, 10, 11),
	Vec3u(3, 11, 4), Vec3u(4, 11, 12),
	Vec3u(4, 12, 5), Vec3u(5, 12, 13),
	Vec3u(5, 13, 6), Vec3u(6, 13, 14),
	Vec3u(6, 14, 7), Vec3u(7, 14, 15),
	Vec3u(7, 15, 0), Vec3u(0, 15, 8),
	// Pyramid Base
	Vec3u(16, 17, 18), Vec3u(16, 17, 19),
	// Pyramid Top
	Vec3u(16, 20, 17), Vec3u(17, 20, 18), Vec3u(18, 20, 19), Vec3u(19, 20, 16)
};

const std::vector<Vec3f> MACE_VERTICES = {
	// Cylinder Base
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
	// Cylinder Top
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),

	// Cube bottom
	Vec3f(2.0f, 0.0f, -2.0f), Vec3f(-2.0f, 0.0f, -2.0f), Vec3f(-2.0f, 0.0f, 2.0f),
	Vec3f(2.0f, 0.0f, 2.0f),
	// Cube top. The shader scales this unit height by tipSize.
	Vec3f(2.0f, 1.0f, -2.0f), Vec3f(-2.0f, 1.0f, -2.0f), Vec3f(-2.0f, 1.0f, 2.0f),
	Vec3f(2.0f, 1.0f, 2.0f)
};

const std::vector<Vec3u> MACE_TRIANGLES = {
	// Clinder Base
	Vec3u(0, 1, 2), Vec3u(0, 2, 3), Vec3u(0, 3, 4), Vec3u(0, 4, 5), Vec3u(0, 5, 6), Vec3u(0, 6, 7),
	// Cyclinder Top
	Vec3u(8, 10, 9), Vec3u(8, 11, 10), Vec3u(8, 12, 11), Vec3u(8, 13, 12), Vec3u(8, 14, 13), Vec3u(8, 15, 14),
	// Cylinder Sides
	Vec3u(0, 8, 1), Vec3u(1, 8, 9),
	Vec3u(1, 9, 2), Vec3u(2, 9, 10),
	Vec3u(2, 10, 3), Vec3u(3, 10, 11),
	Vec3u(3, 11, 4), Vec3u(4, 11, 12),
	Vec3u(4, 12, 5), Vec3u(5, 12, 13),
	Vec3u(5, 13, 6), Vec3u(6, 13, 14),
	Vec3u(6, 14, 7), Vec3u(7, 14, 15),
	Vec3u(7, 15, 0), Vec3u(0, 15, 8),

	// Cube bottom
	Vec3u(16, 18, 17), Vec3u(16, 19, 18),
	// Cube top
	Vec3u(20, 21, 22), Vec3u(20, 22, 23),
	// Cube sides
	Vec3u(16, 17, 21), Vec3u(16, 21, 20),
	Vec3u(19, 23, 22), Vec3u(19, 22, 18),
	Vec3u(17, 18, 22), Vec3u(17, 22, 21),
	Vec3u(16, 20, 23), Vec3u(16, 23, 19)
};

#endif
