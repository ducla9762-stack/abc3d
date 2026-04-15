#ifndef _ABC3D_SHAPES_H_
#define _ABC3D_SHAPES_H_

#include <array>
#include <vector>
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


inline void generateUnitTorus(std::vector<Vec3f>& vertices, std::vector<Vec4u>& faces, uint resolutionU, uint resolutionV, float R = 1.0f, float r = 0.3f) {
	assert(resolutionU >= 3); 
	assert(resolutionV >= 3); 

	uint V = resolutionU * resolutionV; 
	vertices.resize(V); 
	faces.resize(V * 2); 

	for (uint i = 0; i < resolutionU; ++i) {
		float u = cy::Pi<float>() * 2.0f * i / resolutionU; 
		float cosU = std::cos(u); 
		float sinU = std::sin(u); 

		for (uint j = 0; j < resolutionV; ++j) {
			float v = cy::Pi<float>() * 2.0f * j / resolutionV; 
			float cosV = std::cos(v); 
			float sinV = std::sin(v);
			
			uint index = i * resolutionV + j; 

			vertices[index] = Vec3f((R + r * cosV) * cosU,
				r * sinV,
				(R + r * cosV) * sinU
			);
		}
	}

	uint f = 0; 

	for (uint i = 0; i < resolutionU; ++i) {
		uint nextI = (i + 1) % resolutionU; 

		for (uint j = 0; j < resolutionV; ++j) {
			uint nextJ = (j + 1) % resolutionV; 

			uint a = i * resolutionV + j; 
			uint b = nextI * resolutionV + j; 
			uint c = nextI * resolutionV + nextJ; 
			uint d = i * resolutionV + nextJ; 

			faces[f++] = Vec4u(a, b, d, MAX_UINT); 
			faces[f++] = Vec4u(b, c, d, MAX_UINT);
		}
	}
}
#endif //SHAPES_H
