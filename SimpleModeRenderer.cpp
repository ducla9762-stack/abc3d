#include "SimpleModeRenderer.h"

#include <algorithm>
#include <cmath>

namespace {
	Matrix4f OrthographicProjection(float halfHeight, float aspect, float nearPlane, float farPlane) {
		const float halfWidth = halfHeight * aspect;
		Matrix4f projection;
		projection.SetIdentity();
		projection.cell[0] = 1.0f / halfWidth;
		projection.cell[5] = 1.0f / halfHeight;
		projection.cell[10] = -2.0f / (farPlane - nearPlane);
		projection.cell[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
		return projection;
	}
}

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

void SimpleModeRenderer::UpdateCameras(const CameraView& perspective, const Vec3f& target) {
	cameras[0] = perspective;

	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	const Vec3f perspectivePosition = perspective.view.GetInverse().GetTranslation();
	const float perspectiveDistance = std::max((perspectivePosition - target).Length(), 1.0f);
	const float halfHeight = std::max(
		5.0f,
		perspectiveDistance * std::tan(degreesToRadians(perspective.gizmoFov) * 0.5f)
	);
	const float cameraDistance = std::max(1000.0f, halfHeight * 4.0f);
	const float farPlane = cameraDistance * 2.0f;
	const Matrix4f orthographic = OrthographicProjection(halfHeight, aspect, 0.1f, farPlane);
	const float orthographicGizmoFov = radiansToDegrees(2.0f * std::atan(halfHeight));

	const Vec3f cameraAxes[3] = { WORLD_X, WORLD_Y, WORLD_Z };
	const Vec3f upVectors[3] = { WORLD_Y, -WORLD_Z, WORLD_Y };
	const Vec3f gridAxesU[3] = { WORLD_Y, WORLD_X, WORLD_X };
	const Vec3f gridAxesV[3] = { WORLD_Z, WORLD_Z, WORLD_Y };

	for (uint i = 0; i < 3; ++i) {
		const float direction = axisViewNegated[i] ? -1.0f : 1.0f;
		const Vec3f position = target + cameraAxes[i] * (cameraDistance * direction);
		cameras[i + 1].view.SetView(position, target, upVectors[i]);
		cameras[i + 1].projection = orthographic;
		if (axisViewNegated[i]) cameras[i + 1].projection.cell[0] *= -1.0f;
		cameras[i + 1].gizmoFov = orthographicGizmoFov;
		cameras[i + 1].gridAxisU = gridAxesU[i];
		cameras[i + 1].gridAxisV = gridAxesV[i];
		cameras[i + 1].wireframeOnly = true;
	}
}

uint SimpleModeRenderer::CameraIndexAt(const Vec2u& pixel) const {
	const uint splitX = static_cast<uint>(width / 2);
	const uint splitY = static_cast<uint>(height / 2);
	const bool right = pixel.x >= splitX;
	const bool top = pixel.y >= splitY;

	// Top-left: X, top-right: perspective, bottom-left: Y, bottom-right: Z.
	if (top) return right ? 0u : 1u;
	return right ? 3u : 2u;
}

void SimpleModeRenderer::Draw(GLuint defaultFramebuffer, int newWidth, int newHeight,
	const CameraView& perspective, const Vec3f& target, const DrawView& drawView) {
	if (!EnsureFramebuffer(newWidth, newHeight)) {
		cameras[0] = perspective;
		activeCamera = 0;
		drawView(defaultFramebuffer, perspective);
		return;
	}

	UpdateCameras(perspective, target);

	const int splitX = width / 2;
	const int splitY = height / 2;
	const int destinations[4][4] = {
		{ splitX, splitY, width,  height }, // perspective
		{ 0,      splitY, splitX, height }, // X axis
		{ 0,      0,      splitX, splitY }, // Y axis
		{ splitX, 0,      width,  splitY }  // Z axis
	};

	for (uint i = 0; i < cameras.size(); ++i) {
		drawView(framebuffer, cameras[i]);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		const auto& destination = destinations[i];
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

void SimpleModeRenderer::PreparePicking(const Vec2u& displayPixel, int newWidth, int newHeight,
	const CameraView& perspective, const Vec3f& target, const DrawView& drawView) {
	if (!EnsureFramebuffer(newWidth, newHeight)) return;
	UpdateCameras(perspective, target);
	activeCamera = CameraIndexAt(displayPixel);
	drawView(framebuffer, cameras[activeCamera]);
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
