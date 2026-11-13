#pragma once

#define GLEW_STATIC
#include <GL/glew.h>

#include <functional>
#include <array>

#include "helper.h"

// Renders a perspective view and three axis-aligned orthographic views into
// four equal screen quadrants. Scene state remains owned by ABC3DRenderer.
class SimpleModeRenderer {
public:
	struct CameraView {
		Matrix4f view;
		Matrix4f projection;
		float gizmoFov = 70.0f;
	};

	using DrawView = std::function<void(GLuint, const CameraView&)>;

private:
	GLuint framebuffer = 0;
	GLuint colorTexture = 0;
	GLuint depthBuffer = 0;
	int width = 0;
	int height = 0;
	std::array<CameraView, 4> cameras;
	uint activeCamera = 0;

	bool EnsureFramebuffer(int newWidth, int newHeight);
	void UpdateCameras(const CameraView& perspective, const Vec3f& target);
	uint CameraIndexAt(const Vec2u& pixel) const;

public:
	void Draw(GLuint defaultFramebuffer, int width, int height, const CameraView& perspective,
		const Vec3f& target, const DrawView& drawView);
	void PreparePicking(const Vec2u& displayPixel, int width, int height, const CameraView& perspective,
		const Vec3f& target, const DrawView& drawView);

	// Convert a pixel in any displayed quadrant back to the full-size render.
	Vec2u MapPixelToSource(const Vec2u& pixel) const;
	bool IsPerspectivePixel(const Vec2u& pixel) const {
		return width <= 0 || height <= 0 || CameraIndexAt(pixel) == 0;
	}
	const CameraView& ActiveCamera() const { return cameras[activeCamera]; }
};
