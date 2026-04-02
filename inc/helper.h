#ifndef _ABC3D_HELPER_H_
#define _ABC3D_HELPER_H_

#include <iostream>
#include <vector>
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

/// <summary> Convert degrees to radians. </summary>
/// <param name="degrees"> An angle in degrees. </param>
/// <returns> An equivalent angle in radians. </returns>
inline float degreesToRadians(float degrees) {
	return degrees * (cy::Pi<float>() / 180.0f);
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

#endif
