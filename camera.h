#ifndef _ABC3D_CAMERA_H_
#define _ABC3D_CAMERA_H_

#include "helper.h"

enum CameraType { ORBIT, FREE };

class Camera {
public:
	virtual Vec3f position() const = 0;
	virtual Matrix4f transform() const = 0;
	virtual void transform(float data[16]) const = 0;
	virtual void directions(Vec3f& f, Vec3f& u, Vec3f& r) const = 0;
	virtual void mouseDrag(const Vec2f& drag) = 0;
	virtual void mouseWheel(float dx, float dy) = 0;
};

class OrbitCamera : public Camera {
	Vec3f origin = Vec3f(0.0f); // World-Space Center of Orbit
	Vec3f spherical = Vec3f(0.0f); // Local Spherical Coordinates
	Vec3f cartesian = Vec3f(0.0f); // Local Cartesian Coordinates

public:

	OrbitCamera() { }

	void setOrigin(const Vec3f& origin) { this->origin = origin; }
	void setSphericalPosition(const Vec3f& pos) {
		spherical = pos;
		cartesian = sphericalToCartesian(pos);
	}

	Vec3f position() const override { return cartesian + origin; }

	Matrix4f transform() const override { return Matrix4f::View(position(), origin, WORLD_Y); }
	void transform(float data[16]) const override { Matrix4f::View(position(), origin, WORLD_Y).Get(data); }

	void directions(Vec3f& f, Vec3f& u, Vec3f& r) const override {
		f = -cartesian;
		f.Normalize();

		r = WORLD_Y.Cross(f);
		r.Normalize();

		u = r.Cross(f);
		u.Normalize();
	}

	void mouseDrag(const Vec2f& drag) override {
		spherical.x -= drag.x;
		spherical.y += drag.y;
		clamp(spherical.y, 1.0f, 179.0f);

		cartesian = sphericalToCartesian(spherical);
	}

	void mouseWheel(float dx, float dy) override {
		spherical.z -= float(dy);
		if (spherical.z < 0.00001f) { spherical.z = 0.00001f; }

		spherical.x += dx;

		cartesian = sphericalToCartesian(spherical);
	}
};

#endif
