#ifndef _ABC3D_HELPER_H_
#define _ABC3D_HELPER_H_

#undef max

#include <iostream>
#include <vector>
#include <limits>
#include <cmath>

#include "cyVector.h"
#include "cyMatrix.h"

using namespace cy;
using std::string;
using std::vector;

typedef unsigned uint;
typedef Vec2<int> Vec2i;
typedef Vec2<uint> Vec2u;
typedef Vec3<int> Vec3i;
typedef Vec3<uint> Vec3u;
typedef Vec4<int> Vec4i;
typedef Vec4<uint> Vec4u;

#define MAX_UINT std::numeric_limits<uint>::max()

// Useful constants
const Vec3f WORLD_Y = Vec3f(0.0f, 1.0f, 0.0f);
const Vec3f WORLD_X = Vec3f(1.0f, 0.0f, 0.0f);
const Vec3f WORLD_Z = Vec3f(0.0f, 0.0f, 1.0f);
const Vec3f ORIGIN = Vec3f(0.0f);

/// <summary> Convert degrees to radians. </summary>
/// <param name="degrees"> An angle in degrees. </param>
/// <returns> An equivalent angle in radians. </returns>
inline float degreesToRadians(float degrees) {
	return degrees * (cy::Pi<float>() / 180.0f);
}

inline float radiansToDegrees(float radians) {
	return radians * (180.0f / cy::Pi<float>());
}

inline bool compareFloat(float x, float y, float epsilon) {
	float dist = std::abs(x - y);
	return dist < epsilon;
}

inline Vec3f sphericalToCartesian(const Vec3f& coords) {
	float polarAngle = degreesToRadians(coords.x);
	float azimuthalAngle = degreesToRadians(coords.y);
	float polarRadius = std::sin(azimuthalAngle) * coords.z;

	Vec3f cartesian;
	cartesian.x = std::cos(polarAngle) * polarRadius;
	cartesian.y = std::cos(azimuthalAngle) * coords.z;
	cartesian.z = -std::sin(polarAngle) * polarRadius;

	return cartesian;
}

inline void clamp(float& x, float left, float right) {
	if (x < left) { x = left; }
	else if (x > right) { x = right; }
}

inline uint WrapDec(uint x, uint wrapTo, uint wrapFrom = 0) {
	if (x == wrapFrom) {
		return wrapTo;
	}
	return x - 1;
}

inline uint WrapSub(uint x, uint sub, uint wrapTo, uint wrapFrom = 0) {
	uint range = wrapTo - wrapFrom + 1;
	uint offset = (wrapTo - x + sub) % range;
	return wrapTo - offset;
}

inline uint WrapInc(uint x, uint wrapFrom, uint wrapTo = 0) {
	if (x == wrapFrom) {
		return wrapTo;
	}
	return x + 1;
}

inline uint WrapAdd(uint x, uint add, uint wrapFrom) {
	return (x + add) % (wrapFrom + 1);
}

inline uint WrapAdd(uint x, uint add, uint wrapFrom, uint wrapTo) {
	uint range = wrapFrom - wrapTo + 1;
	uint offset = (x + add - wrapTo) % range;
	return wrapTo + offset;
}

inline void combineHash(size_t& base, const size_t& newHash) {
	base = (base << 1) + base + newHash;
}

template<typename T>
struct std::hash<cy::Vec3<T>> {
	size_t operator()(const cy::Vec3<T>& vec) const noexcept {
		std::hash<T> hash;
		size_t val = hash(vec.x);
		combineHash(val, hash(vec.y));
		combineHash(val, hash(vec.z));
		return val;
	}
};

template<typename T>
struct std::hash<cy::Vec2<T>> {
	size_t operator()(const cy::Vec2<T>& vec) const noexcept {
		std::hash<T> hash;
		size_t val = hash(vec.x);
		combineHash(val, hash(vec.y));
		return val;
	}
};

// Vector Output Stream Overloads

inline std::ostream& operator<<(std::ostream& out, const Vec4f& vec) {
	out << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w;
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const Vec4u& vec) {
	out << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w;
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const Vec4i& vec) {
	out << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w;
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const Vec3f& vec) {
	out << vec.x << ", " << vec.y << ", " << vec.z;
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const Vec3u& vec) {
	out << vec.x << ", " << vec.y << ", " << vec.z;
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const Vec3i& vec) {
	out << vec.x << ", " << vec.y << ", " << vec.z;
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const Vec2f& vec) {
	out << vec.x << ", " << vec.y;
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const Vec2u& vec) {
	out << vec.x << ", " << vec.y;
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const Vec2i& vec) {
	out << vec.x << ", " << vec.y;
	return out;
}

inline Vec3f matrixToEulerAngle(const Matrix3f& mat) {
	// Refer to https://www.geometrictools.com/Documentation/EulerAngles.pdf
	
	Vec3f eulerRot;
	eulerRot.y = std::asin(mat.cell[6]);

	constexpr float HALF_PI = Pi<float>() / 2.0f;
	if (eulerRot.y < HALF_PI && eulerRot.y > -HALF_PI) {
		eulerRot.x = std::atan2(-mat.cell[7], mat.cell[8]);
		eulerRot.z = std::atan2(-mat.cell[3], mat.cell[0]);
	}
	else if (eulerRot.y > HALF_PI && eulerRot.y < HALF_PI + 0.0001f) {
		eulerRot.x = std::atan2(mat.cell[1], mat.cell[4]);
		eulerRot.z = 0.0f;
	}
	else if (eulerRot.y < -HALF_PI && eulerRot.y > -HALF_PI - 0.0001f) {
		eulerRot.z = std::atan2(mat.cell[1], mat.cell[4]);
		eulerRot.x = 0.0f;
	}

	eulerRot.x = radiansToDegrees(eulerRot.x);
	eulerRot.y = radiansToDegrees(eulerRot.y);
	eulerRot.z = radiansToDegrees(eulerRot.z);

	return eulerRot;
}

inline Vec3f axisAngleToEulerAngle(const Vec3f& axis, float angle) {
	Matrix3f mat;
	mat.SetRotation(axis, angle);

	return matrixToEulerAngle(mat);
}


#endif
