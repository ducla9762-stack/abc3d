#ifndef _ABC3D_SHAPES_H_
#define _ABC3D_SHAPES_H_

#include <array>
#include <vector>
#include "helper.h"
#include <cmath>

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



/// <summary>
/// This method generates a unit cylinder by modifying the vertices passed in by reference and the faces passed in by reference. 
/// n = resolution. This is for ease of notation in the documentation below.
/// </summary>
/// <typeparam name="F">The number of faces. F = 2*(n - 2) + n</typeparam>
/// <typeparam name="V">The number of vertices. V = n * 2 ALWAYS</typeparam>
/// <param name="vertices">The array for vertices</param>
/// <param name="faces">The array for faces</param>
/// <param name="resolution"> resolution >= 3 ALWAYS. The number of vertices to split the circles for the cylinders into. </param>
template <size_t V, size_t F>
inline void generateUnitCylinder(std::array<Vec3f, V>& vertices, std::array<Vec4u, F>& faces, uint resolution) {
	float theta;
	
	assert(resolution >= 3);
	assert(V == 2 * resolution);
	assert(F == 2 * (resolution - 2) + resolution);

	//Bottom Ring
	for (int i = 0; i < resolution; ++i) {
		theta = cy::Pi<float>() * 2 * i / resolution;
		vertices[i] = Vec3f(std::cos(theta), -1, std::sin(theta));
	}

	//Top Ring
	for (int i = 0; i < resolution; ++i) {
		theta = cy::Pi<float>() * 2 * i / resolution;
		vertices[resolution + i] = Vec3f(std::cos(theta), 1, std::sin(theta));
	}

	//Bottom Cap
	for (int i = 0; i < resolution - 2; ++i)
		faces[i] = Vec4u(0, i+1, i + 2, MAX_UINT);

	//Top Cap
	for (int i = 0; i < resolution - 2; ++i)
		faces[resolution - 2 + i] = Vec4u(resolution, resolution + i + 2, resolution + i + 1, MAX_UINT);
	
	//Side Quads
	int faceOffset = 2 * (resolution - 2); 

	for (int i = 0; i < resolution; ++i) {
		int next = (i + 1) % resolution; 

		faces[faceOffset + i] = Vec4u(i, next, next + resolution, i + resolution);
	}
}

/// <summary>
/// This method generates a unit cylinder by modifying the vertices passed in by reference and the faces passed in by reference. 
/// n = resolution. This is for ease of notation in the documentation below.
/// </summary>
/// <typeparam name="F">The number of faces. F = 2*(n - 2) + n</typeparam>
/// <typeparam name="V">The number of vertices. V = n * 2 ALWAYS</typeparam>
/// <param name="vertices">The array for vertices</param>
/// <param name="faces">The array for faces</param>
/// <param name="resolution"> resolution >= 3 ALWAYS. The number of vertices to split the circles for the cylinders into. </param>
inline void generateUnitCylinder(std::vector<Vec3f>& vertices, std::vector<Vec4u>& faces, uint resolution)
{
	assert(resolution >= 3);

	vertices.resize(2 * resolution);
	faces.resize(2 * (resolution - 2) + resolution);

	float theta;

	// Bottom ring
	for (uint i = 0; i < resolution; ++i) {
		theta = cy::Pi<float>() * 2 * i / resolution;
		vertices[i] = Vec3f(std::cos(theta), -1, std::sin(theta));
	}

	// Top ring
	for (uint i = 0; i < resolution; ++i) {
		theta = cy::Pi<float>() * 2 * i / resolution;
		vertices[resolution + i] = Vec3f(std::cos(theta), 1, std::sin(theta));
	}

	// Bottom cap
	for (uint i = 0; i < resolution - 2; ++i)
		faces[i] = Vec4u(0, i + 1, i + 2, MAX_UINT);

	// Top cap
	for (uint i = 0; i < resolution - 2; ++i)
		faces[resolution - 2 + i] =
		Vec4u(resolution, resolution + i + 2, resolution + i + 1, MAX_UINT);

	// Sides
	uint offset = 2 * (resolution - 2);

	for (uint i = 0; i < resolution; ++i) {
		uint next = (i + 1) % resolution;

		faces[offset + i] =
			Vec4u(i, next, next + resolution, i + resolution);
	}
}

inline void generateUVSphereVertices(std::vector<Vec3f>& vertices, uint latSegments, uint lonSegments) {
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
}

inline void generateUVSphereFaces(std::vector<Vec4u>& faces, uint latSegments, uint lonSegments) {
	faces.reserve(latSegments * lonSegments * 2);

	for (int i = 0; i < latSegments; i++) {
		for (int j = 0; j < lonSegments; j++) {
			int first = i * (lonSegments + 1) + j; 
			int second = first + lonSegments + 1; 

			faces.push_back(Vec4u(first, second, first + 1, MAX_UINT)); 
			faces.push_back(Vec4u(second, second + 1, first + 1, MAX_UINT));
		}
	}
}

inline void generateUVSphere(std::vector<Vec3f>& vertices, std::vector<Vec4u>& faces, uint latitude, uint longitude) {
	generateUVSphereVertices(vertices, latitude, longitude);
	generateUVSphereFaces(faces, latitude, longitude);
}

#endif //SHAPES_H
