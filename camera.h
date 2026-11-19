#ifndef _ABC3D_CAMERA_H_
#define _ABC3D_CAMERA_H_

#include "helper.h"

enum CameraType { ORBIT, FREE };

class Camera {
public:
	virtual Vec3f position() const = 0;
	virtual void setPosition(const Vec3f& pos) = 0;
	virtual Matrix4f transform() const = 0;
	virtual void transform(float data[16]) const = 0;

	virtual Vec3f forward() const = 0;
	virtual void directions(Vec3f& f, Vec3f& u, Vec3f& r) const {
		f = forward();

		r = WORLD_Y.Cross(f);
		r.Normalize();

		u = r.Cross(f);
		u.Normalize();
	}

	virtual Vec3f right() const { Vec3f r = WORLD_Y.Cross(forward()); r.Normalize(); return r; }
	virtual Vec3f right(Vec3f& f) const { f = forward(); Vec3f r = WORLD_Y.Cross(f); r.Normalize(); return r; }
	virtual Vec3f up() const { Vec3f f, u, r; directions(f, u, r); return u; }
	virtual Vec3f up(Vec3f& f, Vec3f& r) const { Vec3f u; directions(f, u, r); return u; }

	virtual void mouseDrag(const Vec2f& drag) = 0;
	virtual void mouseWheel(float dx, float dy) = 0;
	virtual void keyPress(Axis button) = 0;
};

class OrbitCamera : public Camera {
	Vec3f origin = Vec3f(0.0f); // World-Space Center of Orbit
	Vec3f spherical = Vec3f(0.0f, 90.0f, 1.0f); // Local Spherical Coordinates
	Vec3f cartesian = Vec3f(1.0f, 0.0f, 0.0f); // Local Cartesian Coordinates (cached)

public:

	OrbitCamera() { }

	void setOrigin(const Vec3f& origin) { this->origin = origin; }
	Vec3f getOrigin() const { return origin; }
	void setSphericalPosition(const Vec3f& pos) {
		spherical = pos;
		cartesian = sphericalToCartesian(pos);
	}

	Vec3f sphericalPosition() { return spherical; }
	Vec3f position() const override { return cartesian + origin; }

	void setPosition(const Vec3f& pos) override {
		cartesian = pos - origin;

		float radius = cartesian.Length();

		spherical.x = radiansToDegrees(std::acos(pos.x / radius));
		spherical.y = radiansToDegrees(std::acos(pos.y / radius));
		spherical.z = radius;
	}

	Matrix4f transform() const override { Matrix4f m; m.SetView(position(), origin, WORLD_Y); return m; }
	void transform(float data[16]) const override { Matrix4f m; m.SetView(position(), origin, WORLD_Y); m.Get(data); }

	Vec3f forward() const override { return (-cartesian / spherical.z); }

	void mouseDrag(const Vec2f& drag) override {
		spherical.x -= drag.x;
		spherical.y += drag.y;
		clamp(spherical.y, 1.0f, 179.0f);

		cartesian = sphericalToCartesian(spherical);
	}

	void mouseWheel(float dx, float dy) override {
		spherical.z -= dy;
		if (spherical.z < 0.00001f) { spherical.z = 0.00001f; }

		spherical.x += dx;

		cartesian = sphericalToCartesian(spherical);
	}

	void keyPress(Axis button) override {
		constexpr float RADIUS_INC = 0.1f;
		constexpr float ANGLE_INC = 2.0f;
		if (button == X) { spherical.x += ANGLE_INC; }
		else if (button == X_NEG) { spherical.x -= ANGLE_INC;  }

		if (button == Y) { spherical.y += ANGLE_INC; }
		else if (button == Y_NEG) { spherical.y -= ANGLE_INC; }

		if (button == Z) { spherical.z -= RADIUS_INC; }
		else if (button == Z_NEG) { spherical.z += RADIUS_INC; }

		cartesian = sphericalToCartesian(spherical);
		// orbit: change radius with w & s (spherical.z), A&D change first angle (spherical.x)
	}
};

class FreeCamera : public Camera {
	Vec3f pos = Vec3f(0.0f);
	Vec2f dir = Vec2f(0.0f, 90.0f);

public:

	FreeCamera() {}

	void setPosition(const Vec3f& pos) override { this->pos = pos; }
	void setForward(const Vec3f& f) {
		assert(f.LengthSquared() > 1.0f - 0.000001f && f.LengthSquared() < 1.0f + 0.000001f);

		dir.x = radiansToDegrees(std::atan2(-f.z, f.x));
		dir.y = radiansToDegrees(std::acos(f.y));
	}

	Vec3f position() const override { return pos; }
	Vec3f target() const { return pos + sphericalToCartesian(dir); }

	Matrix4f transform() const override { Matrix4f m; m.SetView(pos, target(), WORLD_Y); return m; }
	void transform(float data[16]) const override { Matrix4f m; m.SetView(pos, target(), WORLD_Y); m.Get(data); }

	Vec3f forward() const override { return sphericalToCartesian(dir); }

	void mouseDrag(const Vec2f& drag) override {
		dir.x -= drag.x;
		dir.y -= drag.y;
		clamp(dir.y, 1.0f, 179.0f);
	}

	void mouseWheel(float dx, float dy) override {
		Vec3f f = forward();

		Vec3f r = WORLD_Y.Cross(f);
		r.Normalize();

		pos += r * dx;
		pos += f * -dy;
	}

	void keyPress(Axis button) override {
		constexpr float DIST = 0.1f;
		// Free: get direction from camera, move along directions
		if (button & X) {
			Vec3f r = right();
			if (button & NEG) { r = -r; }
			pos += r * DIST;
		}
		else if (button & Z) {
			Vec3f f = forward();
			if (button & NEG) { f = -f; }
			pos += f * DIST;
		}
		// Add a way to change direction
	}
};

#endif
