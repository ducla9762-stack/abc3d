#ifndef _ABC3D_SHAPES_H_
#define _ABC3D_SHAPES_H_

#include <array>
#include "helper.h"

const std::array<Vec3f, 8> CUBE_VERTICES = { 
	Vec3f(1.0f, 1.0f, -1.0f),
	Vec3f(1.0f, -1.0f, -1.0f),
	Vec3f(1.0f, 1.0f, 1.0f),
	Vec3f(1.0f, -1.0f, 1.0f),
	Vec3f(-1.0f, 1.0f, -1.0f),
	Vec3f(-1.0f, -1.0f, -1.0f),
	Vec3f(-1.0f, 1.0f, 1.0f),
	Vec3f(-1.0f, -1.0f, 1.0f) 
};

const std::array<Vec4u, 6> CUBE_FACES = { 
	Vec4u(0,4,6,2),
	Vec4u(3,2,6,7),
	Vec4u(7,6,4,5),
	Vec4u(5,1,3,7),
	Vec4u(1,0,2,3),
	Vec4u(5,4,0,1) 
}; 

const std::array<Vec3f, 5> PYRAMID_VERTICES = {
	Vec3f(-1.0f, 0.0f, 1.0f),
	Vec3f(-1.0f, 0.0f, -1.0f),
	Vec3f(1.0f, 0.0f, -1.0f),
	Vec3f(1.0f, 0.0f, 1.0f),
	Vec3f(0.0f, 2.0f, 0.0f)
};

const std::array<Vec4u, 5> PYRAMID_FACES = { 
	Vec4u(0, 1, 2, 3), 
	Vec4u(1, 0, 4, MAX_UINT), 
	Vec4u(0, 3, 4, MAX_UINT), 
	Vec4u(3, 2, 4, MAX_UINT), 
	Vec4u(1, 4, 2, MAX_UINT)
};

#endif //SHAPES_H
