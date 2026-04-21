#include <GL/glew.h>

#include <iostream>
#include <chrono>

#include "cyGL.h"
#include "cyMatrix.h"

#include "helper.h"
#include "scene.h"
#include "mesh.h"
#include "render.h"

void ModelingRenderer::ToggleGrid() { displayGrid = !displayGrid; }
void ModelingRenderer::ToggleVertexPoints() { displayPoints = !displayPoints; }

Vec3f ModelingRenderer::GetCamPos() { return camPos; }
Vec3f ModelingRenderer::GetCamTarget() { return camTarget; }

void ModelingRenderer::SetCamPos(const Vec3f& pos) { camPos = pos; updateUniforms(); }
void ModelingRenderer::SetCamTarget(const Vec3f& target) { camTarget = target; updateUniforms(); }

Vec3f ModelingRenderer::GetCamUp() {
	Vec3f forward = camTarget - camPos;
	Vec3f right = WORLD_Y.Cross(forward);
	Vec3f up = right.Cross(forward);
	up.Normalize();
	return up;
}

Vec3f ModelingRenderer::GetCamRight() {
	Vec3f right = WORLD_Y.Cross(camTarget - camPos);
	right.Normalize();
	return right;
}

void ModelingRenderer::SetCamera(const Vec3f& pos, const Vec3f& target) {
	camPos = pos;
	camTarget = target;
	updateUniforms();
}

Vec3u ModelingRenderer::SelectionIds(uint x, uint y) { return idSelectBuf.at(x + y * width); }

bool ModelingRenderer::Initialize() {
	// Create VAO for grid
	glGenVertexArrays(1, &gridVAO);

	// Create VAO for vertex points
	glGenVertexArrays(1, &pointVAO);
	glBindVertexArray(pointVAO);

	glGenBuffers(1, &pointVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pointVBO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glGenBuffers(1, &pointIdVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pointIdVBO);

	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, 0, (GLvoid*)0);

	// Create VAO for mesh
	glGenVertexArrays(1, &meshVAO);
	glBindVertexArray(meshVAO);

	glGenBuffers(1, &meshVBO);
	glBindBuffer(GL_ARRAY_BUFFER, meshVBO);

	//updateRenderBuffers();

	// Create pixel buffer object for pixel streaming
	glGenBuffers(1, &pbo);

	// Create textures
	glGenTextures(1, &idTex);
	glGenTextures(1, &clrTex);
	glGenTextures(1, &selTex);

	// Initialize Shader Programs
	bool success = gridProgram.BuildFiles("../assets/shaders/Grid.vert", "../assets/shaders/Grid.frag");
	if (!success) {
		std::cout << "Failed to compile grid shader program!" << std::endl;
		//Fl::hide_all_windows();
		return false;
	}

	success = meshProgram.BuildFiles("../assets/shaders/Mesh.vert", "../assets/shaders/Mesh.frag");
	if (!success) {
		std::cout << "Failed to compile mesh shader program!" << std::endl;
		//Fl::hide_all_windows();
		return false;
	}

	meshProgram.Bind();
	meshProgram.SetUniform("lightPosition", 0.0f, 0.0f, 0.0f);

	success = pointProgram.BuildFiles("../assets/shaders/PointDrawing.vert", "../assets/shaders/StaticColor.frag");
	if (!success) {
		std::cout << "Failed to compile wireframe shader program!" << std::endl;
		//Fl::hide_all_windows();
		return false;
	}

	pointProgram.Bind();
	pointProgram.SetUniform("scale", pointScale);

	updateUniforms();
	// Initialize the selection boolean for the mesh shader program.
	// By default nothing should be selected.

	// Set clear color & blend function
	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPointSize(20.0f);

	return true;
}

void ModelingRenderer::Draw() {
	glFinish();
	steady_clock::time_point start = steady_clock::now();

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(meshVAO);
	meshProgram.Bind();
	meshProgram.Bind();

	glDrawArrays(GL_TRIANGLES, 0, meshVertDrawCount);

	if (displayPoints) {
		glBindVertexArray(pointVAO);
		pointProgram.Bind();
		glDrawArrays(GL_POINTS, 0, pointVertDrawCount);
	}

	glReadBuffer(GL_COLOR_ATTACHMENT1);
	glReadPixels(0, 0, width, height, GL_RGB_INTEGER, GL_UNSIGNED_INT, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	if (displayGrid) {
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);

		glBindVertexArray(gridVAO);
		gridProgram.Bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}

	void* selectBuf = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (selectBuf) {
		uint selectBufSize = width * height;
		idSelectBuf.resize(0);
		Vec3u* idSelectBuffer = (Vec3u*)selectBuf;
		idSelectBuf.insert(idSelectBuf.begin(), idSelectBuffer, idSelectBuffer + selectBufSize);
	}
	else {
		std::cout << "ERROR with mapping" << std::endl;
	}
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	/*int selectBufSize = pixel_w() * pixel_h();

	glReadBuffer(GL_COLOR_ATTACHMENT1);
	posSelectBuf.resize(selectBufSize);
	glReadPixels(0, 0, pixel_w(), pixel_h(), GL_RGB, GL_FLOAT, posSelectBuf.data());

	glReadBuffer(GL_COLOR_ATTACHMENT2);
	idSelectBuf.resize(selectBufSize);
	glReadPixels(0, 0, pixel_w(), pixel_h(), GL_RGB_INTEGER, GL_UNSIGNED_INT, idSelectBuf.data());

	glReadBuffer(GL_COLOR_ATTACHMENT0);*/

	glFinish();
	steady_clock::time_point end = steady_clock::now();
	milliseconds ms = duration_cast<milliseconds>(end - start);
	std::cout << "Milliseconds: " << ms.count() << std::endl;
}

void ModelingRenderer::Resize(int width, int height) {
	this->width = width;
	this->height = height;
	SetPerspectiveAndViewport();
	InitFramebuffer();
}

void ModelingRenderer::SetPerspectiveAndViewport() {
	projection.SetPerspective(degreesToRadians(fov), float(width) / float(height), 0.1f, 1000.0f);
	UpdateProjectionUniforms();

	glViewport(0, 0, width, height);
}

void ModelingRenderer::UpdateProjectionUniforms() {
	float data[16];

	projection.Get(data);
	meshProgram.Bind();
	meshProgram.SetUniformMatrix4("projection", data);

	Matrix4f viewProj = projection * viewTransform;
	viewProj.Get(data);
	gridProgram.Bind();
	gridProgram.SetUniformMatrix4("mvp", data);
	pointProgram.Bind();
	pointProgram.SetUniformMatrix4("mvp", data);
}

void ModelingRenderer::updateUniforms() {
	viewTransform.SetView(camPos, camTarget, WORLD_Y);

	float data[16];
	viewTransform.Get(data);
	meshProgram.Bind();
	meshProgram.SetUniformMatrix4("modelView", data);

	Matrix3f normalTransform(viewTransform);
	normalTransform.Invert();
	normalTransform.Transpose();
	normalTransform.Get(data);
	meshProgram.SetUniformMatrix3("normalTransform", data);

	Matrix4f viewProj = projection * viewTransform;
	viewProj.Get(data);
	gridProgram.Bind();
	gridProgram.SetUniformMatrix4("mvp", data);
	pointProgram.Bind();
	pointProgram.SetUniformMatrix4("mvp", data);
}

void ModelingRenderer::updateTextureUniforms() {
	meshProgram.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, selTex);
	meshProgram.SetUniform("selectionTex", 0);
}

void ModelingRenderer::InitFramebuffer() {
	// Delete old framebuffer
	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &fboDepth);

	// Create new framebuffer
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Allocate textures and render buffer

	glGenRenderbuffers(1, &fboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, fboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

	glBindTexture(GL_TEXTURE_2D, clrTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, idTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);

	glBindTexture(GL_TEXTURE_1D, selTex);
	// Start with 4 entries.
	glTexImage1D(GL_TEXTURE_1D, 0, GL_R8UI, 4, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Allocate pixel buffer objects for streaming
	uint pboSize = width * height * 3 * (sizeof(float) + sizeof(uint));

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, pboSize, NULL, GL_STREAM_READ);

	// Attach textures and depth buffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepth);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, clrTex, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, idTex, 0);

	// Enable all color attachments for drawing
	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);

	// Make sure framebuffer is valid
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer is borked!" << std::endl;
	}

	// Re-bind default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ModelingRenderer::UpdateRenderBuffers(const Scene& scene) {
	// Unique vertex data for drawing vertex points
	sceneVertices.resize(0);
	pointVertDrawCount = 0;
	for (unsigned int i = 0; i < scene.MeshCount(); i++)
	{
		const HalfEdgeMesh* mesh = scene[i];
		vertObjData.insert(vertObjData.begin(), mesh->VertexCount(), Vec2u(i + 1, pointVertDrawCount));
		pointVertDrawCount += mesh->VertexCount();
		mesh->GetWorldVertexPositions(sceneVertices);
	}
	glBindVertexArray(pointVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pointVertDrawCount * 3, sceneVertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, pointIdVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uint) * pointVertDrawCount * 2, vertObjData.data(), GL_DYNAMIC_DRAW);

	// Mesh Data
	meshPosData.resize(0);
	meshNormData.resize(0);
	meshIdData.resize(0);
	meshEdgeIdData.resize(0);
	meshDistData.resize(0);
	for (unsigned int i = 0; i < scene.MeshCount(); i++)
	{
		const HalfEdgeMesh* mesh = scene[i];
		mesh->getRenderBuffers(meshPosData, meshNormData, meshIdData, meshDistData, meshEdgeIdData, i + 1);
	}

	size_t meshPosDataSize = sizeof(float) * 3 * meshPosData.size();
	size_t meshNormDataSize = sizeof(float) * 3 * meshNormData.size();
	size_t meshIdDataSize = sizeof(uint) * 2 * meshIdData.size();
	size_t meshDistDataSize = sizeof(float) * meshDistData.size();
	size_t meshEdgeIdDataSize = sizeof(uint) * 3 * meshEdgeIdData.size();
	size_t bufOffset1 = meshPosDataSize + meshNormDataSize;
	size_t bufOffset2 = bufOffset1 + meshIdDataSize;
	size_t bufOffset3 = bufOffset2 + meshDistDataSize;
	size_t size = bufOffset3 + meshEdgeIdDataSize;

	meshVertDrawCount = meshPosData.size();

	glBindVertexArray(meshVAO);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
	glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, meshPosDataSize, meshPosData.data());
	glBufferSubData(GL_ARRAY_BUFFER, meshPosDataSize, meshNormDataSize, meshNormData.data());
	glBufferSubData(GL_ARRAY_BUFFER, bufOffset1, meshIdDataSize, meshIdData.data());
	glBufferSubData(GL_ARRAY_BUFFER, bufOffset2, meshDistDataSize, meshDistData.data());
	glBufferSubData(GL_ARRAY_BUFFER, bufOffset3, meshEdgeIdDataSize, meshEdgeIdData.data());

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)meshPosDataSize);

	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 2, GL_UNSIGNED_INT, 0, (GLvoid*)bufOffset1);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, (GLvoid*)bufOffset2);

	glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 3, GL_UNSIGNED_INT, 0, (GLvoid*)bufOffset3);

	updateTextureUniforms();
}

void ModelingRenderer::UpdateSelectionTexture(std::vector<bool> selections)
{
	std::vector<uint8_t> data(selections.size());

	for (size_t i = 0; i < selections.size(); i++) {
		data[i] = selections[i] ? 1 : 0;
	}

	glBindTexture(GL_TEXTURE_1D, selTex);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_R8UI, data.size(), 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, data.data());
}