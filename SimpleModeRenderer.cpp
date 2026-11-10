#include "SimpleModeRenderer.h"

#include <algorithm>

bool SimpleModeRenderer::EnsureFramebuffer(int newWidth, int newHeight) {
	if (newWidth <= 0 || newHeight <= 0) return false;
	if (framebuffer != 0 && width == newWidth && height == newHeight) return true;

	GLint previousDrawFramebuffer = 0;
	GLint previousReadFramebuffer = 0;
	GLint previousRenderbuffer = 0;
	GLint previousActiveTexture = GL_TEXTURE0;
	GLint previousTexture = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

	const GLuint oldColorTexture = colorTexture;
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteTextures(1, &colorTexture);
	glDeleteRenderbuffers(1, &depthBuffer);
	if (static_cast<GLuint>(previousTexture) == oldColorTexture) previousTexture = 0;

	width = newWidth;
	height = newHeight;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	const GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &drawBuffer);
	const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	if (!complete) {
		std::cout << "Simple mode framebuffer is invalid!" << std::endl;
	}

	glActiveTexture(static_cast<GLenum>(previousActiveTexture));
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
	glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(previousRenderbuffer));
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFramebuffer));
	glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));
	return complete;
}

void SimpleModeRenderer::Draw(GLuint defaultFramebuffer, int newWidth, int newHeight,
	const std::function<void(GLuint)>& drawRegularViewport) {
	if (!EnsureFramebuffer(newWidth, newHeight)) {
		drawRegularViewport(defaultFramebuffer);
		return;
	}

	// Produce the exact standard-mode frame first, including reference images,
	// gizmos, and other overlays, then present that frame in each quadrant.
	drawRegularViewport(framebuffer);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	const int splitX = width / 2;
	const int splitY = height / 2;
	const int destinations[4][4] = {
		{ 0,      0,      splitX, splitY },
		{ splitX, 0,      width,  splitY },
		{ 0,      splitY, splitX, height },
		{ splitX, splitY, width,  height }
	};

	for (const auto& destination : destinations) {
		if (destination[0] == destination[2] || destination[1] == destination[3]) continue;
		glBlitFramebuffer(
			0, 0, width, height,
			destination[0], destination[1], destination[2], destination[3],
			GL_COLOR_BUFFER_BIT, GL_LINEAR
		);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
	glViewport(0, 0, width, height);
}

Vec2u SimpleModeRenderer::MapPixelToSource(const Vec2u& pixel) const {
	if (width <= 0 || height <= 0) return pixel;

	const uint splitX = static_cast<uint>(width / 2);
	const uint splitY = static_cast<uint>(height / 2);
	const uint sourceWidth = static_cast<uint>(width);
	const uint sourceHeight = static_cast<uint>(height);

	const uint originX = pixel.x < splitX ? 0u : splitX;
	const uint originY = pixel.y < splitY ? 0u : splitY;
	const uint quadrantWidth = pixel.x < splitX ? splitX : sourceWidth - splitX;
	const uint quadrantHeight = pixel.y < splitY ? splitY : sourceHeight - splitY;

	const uint mappedX = quadrantWidth <= 1
		? 0u
		: ((pixel.x - originX) * (sourceWidth - 1) + (quadrantWidth - 1) / 2) / (quadrantWidth - 1);
	const uint mappedY = quadrantHeight <= 1
		? 0u
		: ((pixel.y - originY) * (sourceHeight - 1) + (quadrantHeight - 1) / 2) / (quadrantHeight - 1);

	return Vec2u(std::min(mappedX, sourceWidth - 1), std::min(mappedY, sourceHeight - 1));
}
