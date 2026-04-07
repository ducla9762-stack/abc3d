#ifndef SHAPES_H
#define SHAPES_H

#include <vector>
#include "helper.h"

//const uint MAX_UINT = std::numeric_limits<uint>::max();
#define MAX_UINT std::numeric_limits<uint>::max()

const std::vector<Vec3f> CUBE_VERTICES = { Vec3f(1.0f, 1.0f, -1.0f),
Vec3f(1.0f, -1.0f, -1.0f),
Vec3f(1.0f, 1.0f, 1.0f),
Vec3f(1.0f, -1.0f, 1.0f),
Vec3f(-1.0f, 1.0f, -1.0f),
Vec3f(-1.0f, -1.0f, -1.0f),
Vec3f(-1.0f, 1.0f, 1.0f),
Vec3f(-1.0f, -1.0f, 1.0f) };

const std::vector<Vec4u> CUBE_FACES = { Vec4u(0,4,6,2),
Vec4u(3,2,6,7),
Vec4u(7,6,4,5),
Vec4u(5,1,3,7),
Vec4u(1,0,2,3),
Vec4u(5,4,0,1) }; 

const std::vector<Vec3f> PYRAMID_VERTICES = {
	Vec3f(-1.0f, 0.0f, 1.0f),
	Vec3f(-1.0f, 0.0f, -1.0f),
	Vec3f(1.0f, 0.0f, -1.0f),
	Vec3f(1.0f, 0.0f, 1.0f),
	Vec3f(0.0f, 2.0f, 0.0f)
};

const std::vector<Vec4u> PYRAMID_FACES = { 
Vec4u(0, 1, 2, 3), 
Vec4u(1, 0, 4, MAX_UINT), 
Vec4u(0, 3, 4, MAX_UINT), 
Vec4u(3, 2, 4, MAX_UINT), 
Vec4u(1, 4, 2, MAX_UINT)};

//const std::vector<Vec3f> PYRAMID_VERTICES = {
//	Vec3f(1.0f, 0.0f, 1.0f),
//	Vec3f(1.0f, 0.0f, -1.0f),
//	Vec3f(-1.0f, 0.0f, -1.0f),
//	Vec3f(-1.0f, 0.0f, 1.0f),
//	Vec3f(0.0f, 2.0f, 0.0f)
//};
//
//const std::vector<Vec4u> PYRAMID_FACES = { 
//Vec4u(0, 3, 2, 1), 
//Vec4u(0, 4, 1, MAX_UINT), 
//Vec4u(1, 4, 2, MAX_UINT), 
//Vec4u(2, 4, 3, MAX_UINT), 
//Vec4u(3, 4, 1, MAX_UINT)};

#endif //SHAPES_H
