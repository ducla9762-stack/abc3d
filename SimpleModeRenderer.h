#pragma once

#define GLEW_STATIC
#include <GL/glew.h>

#include <functional>

#include "helper.h"

// Composites one normal viewport render into four equal screen quadrants.
// The normal renderer remains the single owner of scene and camera state.
class SimpleModeRenderer {
	GLuint framebuffer = 0;
	GLuint colorTexture = 0;
	GLuint depthBuffer = 0;
	int width = 0;
	int height = 0;

	bool EnsureFramebuffer(int newWidth, int newHeight);

public:
	void Draw(GLuint defaultFramebuffer, int width, int height,
		const std::function<void(GLuint)>& drawRegularViewport);

	// Convert a pixel in any displayed quadrant back to the full-size render.
	Vec2u MapPixelToSource(const Vec2u& pixel) const;
};
