#ifndef SHAPES_H
#define SHAPES_H

#include <vector>
#include "helper.h"

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

const std::vector<Vec3f> ICOSPHERE_VERTICES = {
	Vec3f(0.0f, -1.0f, 0.0f),
	Vec3f(0.7236f,-0.447215f, 0.52572f),
	Vec3f(-0.276385f, -0.447215f, 0.85064f),
	Vec3f(-0.894425f, -0.447215f, 0.0f),
	Vec3f(-0.276385f, -0.447215f, -0.85064f),
	Vec3f(0.7236f, -0.447215f, -0.52572f),
	Vec3f(0.276385f, 0.447215f, 0.85064f),
	Vec3f(-0.723600, 0.447215, 0.52572f),
	Vec3f(-0.7236f, 0.447215f, -0.52572f),
	Vec3f(0.276385f, 0.447215f, -0.85064f),
	Vec3f(0.894425f, 0.447215f, 0.0f),
	Vec3f(0.0f, 1.0f, 0.0f)
};

const std::vector<Vec4u> ICOSPHERE_FACES = {
Vec4u(0, 1, 2, MAX_UINT),
Vec4u(1, 0, 5, MAX_UINT),
Vec4u(0, 2, 3, MAX_UINT),
Vec4u(0, 3, 4, MAX_UINT),
Vec4u(0, 4, 5, MAX_UINT),
Vec4u(1, 5, 10, MAX_UINT),
Vec4u(2, 1, 6, MAX_UINT),
Vec4u(3, 2, 7, MAX_UINT),
Vec4u(4, 3, 8, MAX_UINT),
Vec4u(5, 4, 9, MAX_UINT),
Vec4u(1, 10, 6, MAX_UINT),
Vec4u(2, 6, 7, MAX_UINT),
Vec4u(3, 7, 8, MAX_UINT),
Vec4u(4, 8, 9, MAX_UINT),
Vec4u(5, 9, 10, MAX_UINT),
Vec4u(6, 10, 11, MAX_UINT),
Vec4u(7, 6, 11, MAX_UINT),
Vec4u(8, 7, 11, MAX_UINT),
Vec4u(9, 8, 11, MAX_UINT),
Vec4u(10, 9, 11, MAX_UINT)
};

std::vector<Vec3f> generateUVSphereVertices(uint latSegments, uint lonSegments) {
	std::vector<Vec3f> vertices; 
	vertices.reserve((latSegments + 1) * (lonSegments + 1));

	for (int i = 0; i <= latSegments; i++) {
		float v = float(i) / latSegments; 
		float phi = v * cy::Pi<float>();

		for (int j = 0; j <= lonSegments; j++) {
			float u = (float)j / lonSegments; 
			float theta = u * 2.0f * cy::Pi<float>(); 

			float x = std::sin(phi) * std::cos(theta); 
			float y = std::cos(phi); 
			float z = std::sin(phi) * std::sin(theta);

			vertices.push_back(Vec3f(x, y, z)); 
		}
	}

	return vertices; 
}

std::vector<Vec4u> generateUVSphereFaces(uint latSegments, uint lonSegments) {
	std::vector<Vec4u> faces; 
	faces.reserve(latSegments * lonSegments * 2);

	for (int i = 0; i < latSegments; i++) {
		for (int j = 0; j < lonSegments; j++) {
			int first = i * (lonSegments + 1) + j; 
			int second = first + lonSegments + 1; 

			faces.push_back(Vec4u(first, second, first + 1, MAX_UINT)); 
			faces.push_back(Vec4u(second, second + 1, first + 1, MAX_UINT));
		}
	}

	return faces; 
}
#endif //SHAPES_H
