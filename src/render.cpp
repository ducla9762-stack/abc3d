#include <GL/glew.h>

#include <iostream>
#include <chrono>
#include <array>
#include <algorithm>

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

void ModelingRenderer::GetCameraDirs(Vec3f& forward, Vec3f& up, Vec3f& right) {
	forward = camTarget - camPos;
	right = WORLD_Y.Cross(forward);
	right.Normalize();
	up = right.Cross(forward);
	up.Normalize();
}

void ModelingRenderer::SetCamera(const Vec3f& pos, const Vec3f& target) {
	camPos = pos;
	camTarget = target;
	updateUniforms();
}

Vec3u ModelingRenderer::SelectionIds(uint x, uint y) {
	if (x >= static_cast<uint>(width) || y >= static_cast<uint>(height) || idSelectBuf.empty()) {
		return Vec3u(0u, 0u, 0u);
	}
	return idSelectBuf.at(x + y * width);
}

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

	// Initialize arrow renderer
	arrowRenderer.Initialize();
	arrowRenderer.SetArrows({});

	// Set clear color & blend function
	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPointSize(10.0f);

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

	// Draw the arrows before checking the selection texture.
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	arrowRenderer.Draw();
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

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

	//arrowRenderer.Draw();

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

	arrowRenderer.SetViewProjectionUniform(data, fov, height);
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

	arrowRenderer.SetViewProjectionUniform(data, fov, height);
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
	vertObjData.resize(0);
	pointVertDrawCount = 0;
	for (unsigned int i = 0; i < scene.MeshCount(); i++)
	{
		const HalfEdgeMesh* mesh = scene[i];
		vertObjData.insert(vertObjData.end(), mesh->VertexCount(), Vec2u(i + 1, pointVertDrawCount));
		pointVertDrawCount += mesh->VertexCount();
		mesh->GetWorldVertexPositions(sceneVertices);
	}
	glBindVertexArray(pointVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pointVertDrawCount * 3, sceneVertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, pointIdVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uint) * pointVertDrawCount * 2, vertObjData.data(), GL_DYNAMIC_DRAW);

	// Mesh Data
	bufBuilder.Clear();

	for (unsigned int i = 0; i < scene.MeshCount(); i++)
	{
		const HalfEdgeMesh* mesh = scene[i];
		bufBuilder.AddMesh(*mesh, i + 1);
	}

	size_t meshPosDataSize = sizeof(float) * 3 * bufBuilder.PosBuffer().size();
	size_t meshNormDataSize = sizeof(float) * 3 * bufBuilder.NormBuffer().size();
	size_t meshIdDataSize = sizeof(uint) * 2 * bufBuilder.IdBuffer().size();
	size_t meshDistDataSize = sizeof(float) * bufBuilder.EdgeDistBuffer().size();
	size_t meshEdgeIdDataSize = sizeof(uint) * 3 * bufBuilder.EdgeIdBuffer().size();
	size_t bufOffset1 = meshPosDataSize + meshNormDataSize;
	size_t bufOffset2 = bufOffset1 + meshIdDataSize;
	size_t bufOffset3 = bufOffset2 + meshDistDataSize;
	size_t size = bufOffset3 + meshEdgeIdDataSize;

	meshVertDrawCount = bufBuilder.PosBuffer().size();

	glBindVertexArray(meshVAO);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
	glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, meshPosDataSize, bufBuilder.PosBuffer().data());
	glBufferSubData(GL_ARRAY_BUFFER, meshPosDataSize, meshNormDataSize, bufBuilder.NormBuffer().data());
	glBufferSubData(GL_ARRAY_BUFFER, bufOffset1, meshIdDataSize, bufBuilder.IdBuffer().data());
	glBufferSubData(GL_ARRAY_BUFFER, bufOffset2, meshDistDataSize, bufBuilder.EdgeDistBuffer().data());
	glBufferSubData(GL_ARRAY_BUFFER, bufOffset3, meshEdgeIdDataSize, bufBuilder.EdgeIdBuffer().data());

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

void ModelingRenderer::UpdateArrows(const Vec3f& base, uint hoveredAxis) {
	arrowRenderer.SetArrows(arrowRenderer.CreateArrows(base, hoveredAxis));
}

void ModelingRenderer::ClearArrows() {
	arrowRenderer.SetArrows({});
}

// OLD FUNCTION
void ModelingRenderer::UpdateSelectionTexture(std::vector<bool> selections)
{
	std::vector<uint8_t> data(selections.size());

	for (size_t i = 0; i < selections.size(); i++) {
		data[i] = selections[i] ? 1 : 0;
	}

	glBindTexture(GL_TEXTURE_1D, selTex);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_R8UI, data.size(), 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, data.data());
}

void ModelingRenderer::UpdateSelection(const std::vector<SelectionItem>& selectedItems)
{
	// Pack the CPU selection list into the format used by the GLSL array:
	// (element type, one-based GPU object ID, local component ID).
	std::array<GLuint, MAX_SELECTION_ITEMS * 3> gpuSelections{};
	const size_t selectionCount = selectedItems.size();

	for (size_t i = 0; i < selectionCount; ++i) {
		const SelectionItem& item = selectedItems[i];
		gpuSelections[i * 3 + 0] = static_cast<GLuint>(item.type);
		gpuSelections[i * 3 + 1] = item.meshId + 1; // GPU IDs are one-based
		gpuSelections[i * 3 + 2] = item.componentId;
	}

	// Uniform arrays are uploaded with the location of their first element.
	meshProgram.Bind();
	meshProgram.SetUniform("selectionCount", static_cast<GLuint>(selectionCount));
	meshProgram.SetUniform3(
		"selectedItems[0]", gpuSelections.data(), static_cast<int>(selectionCount));

	pointProgram.Bind();
	pointProgram.SetUniform("selectionCount", static_cast<GLuint>(selectionCount));
	pointProgram.SetUniform3(
		"selectedItems[0]", gpuSelections.data(), static_cast<int>(selectionCount));
}

// AI-generated code: Converts framebuffer IDs into shader hover state.
void ModelingRenderer::UpdateHover(const Vec3u& selectionIds, Element activeElement) {
	GLuint hoveredItem[3] = { MAX_UINT, MAX_UINT, MAX_UINT };

	// Arrows and the background must not produce a mesh-element hover.
	if (selectionIds.x != 0 && selectionIds.x != MAX_UINT && selectionIds.x != ARROW_IDENTIFIER) {
		switch (activeElement) {
		case OBJECT:
			hoveredItem[0] = OBJECT;
			hoveredItem[1] = selectionIds.x;
			break;
		case FACE:
			hoveredItem[0] = FACE;
			hoveredItem[1] = selectionIds.x;
			hoveredItem[2] = selectionIds.y;
			break;
		case EDGE:
			if (selectionIds.z != MAX_UINT) {
				hoveredItem[0] = EDGE;
				hoveredItem[1] = selectionIds.x;
				hoveredItem[2] = selectionIds.z;
			}
			break;
		case VERTEX:
			break;
		}
	}
	else if (activeElement == OBJECT && selectionIds.x == MAX_UINT && selectionIds.y != 0) {
		// Vertex-point rendering stores the object ID in the second component.
		hoveredItem[0] = OBJECT;
		hoveredItem[1] = selectionIds.y;
	}
	else if (activeElement == VERTEX && selectionIds.x == MAX_UINT && selectionIds.y != 0) {
		// Vertex-point rendering stores (object ID, vertex ID) in y/z.
		hoveredItem[0] = VERTEX;
		hoveredItem[1] = selectionIds.y;
		hoveredItem[2] = selectionIds.z;
	}

	meshProgram.Bind();
	meshProgram.SetUniform3("hoveredItem", hoveredItem);
	pointProgram.Bind();
	pointProgram.SetUniform3("hoveredItem", hoveredItem);
}

// Arrow renderer

bool ArrowRenderer::Initialize() {
	// Build shader program
	bool success = program.BuildFiles("../assets/shaders/Arrow.vert", "../assets/shaders/ArrowColor.frag");
	if (!success) {
		std::cout << "Failed to compile arrow shader programs!" << std::endl;
		return false;
	}

	// Generate vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Generate VBO for generic arrow vertices
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * VERTICES.size() * 3, VERTICES.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	// Generate EBO for generic arrow faces
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * TRIANGLES.size() * 3, TRIANGLES.data(), GL_STATIC_DRAW);

	// Generate VBO for instanced data (Arrow struct)
	glGenBuffers(1, &instancedVbo);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVbo);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Arrow), (GLvoid*)offsetof(Arrow, base));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Arrow), (GLvoid*)offsetof(Arrow, dir));
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Arrow), (GLvoid*)offsetof(Arrow, length));
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Arrow), (GLvoid*)offsetof(Arrow, radius));
	glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Arrow), (GLvoid*)offsetof(Arrow, tipHeight));
	glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(Arrow), (GLvoid*)offsetof(Arrow, color));
	glVertexAttribIPointer(7, 2, GL_UNSIGNED_INT, sizeof(Arrow), (GLvoid*)offsetof(Arrow, ids));

	for (int i = 1; i < 8; ++i) {
		glEnableVertexAttribArray(i);
		glVertexAttribDivisor(i, 1);
	}

	return true;
}

void ArrowRenderer::SetArrows(const std::vector<Arrow>& arrows, GLenum usage) {
	glBindBuffer(GL_ARRAY_BUFFER, instancedVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Arrow) * arrows.size(), arrows.data(), usage);
	instanceCount = arrows.size();
}

// AI-generated code: Builds the translation-axis gizmo and applies hover color.
std::vector<ArrowRenderer::Arrow> ArrowRenderer::CreateArrows(const Vec3f& base, uint hoveredAxis) {
	std::vector<ArrowRenderer::Arrow> arrows;
	// Translation, ID = 0.
	// X - Green, Sub-ID = 0
	arrows.emplace_back();
	arrows.back().base = base;
	arrows.back().dir = Vec3f(1.0f, 0.0f, 0.0f);
	arrows.back().length = 0.5f;
	arrows.back().radius = 0.025f;
	arrows.back().tipHeight = 0.25f;
	arrows.back().color = hoveredAxis == 0 ? Vec3f(1.0f, 1.0f, 0.0f) : Vec3f(0.0f, 1.0f, 0.0f);
	arrows.back().ids = Vec2u(0, 0);
	// Y (up) - Blue, Sub-ID = 1
	arrows.emplace_back();
	arrows.back().base = base;
	arrows.back().dir = Vec3f(0.0f, 1.0f, 0.0f);
	arrows.back().length = 0.5f;
	arrows.back().radius = 0.025f;
	arrows.back().tipHeight = 0.25f;
	arrows.back().color = hoveredAxis == 1 ? Vec3f(1.0f, 1.0f, 0.0f) : Vec3f(0.0f, 0.0f, 1.0f);
	arrows.back().ids = Vec2u(0, 1);
	// Z - Red, Sub-ID = 2
	arrows.emplace_back();
	arrows.back().base = base;
	arrows.back().dir = Vec3f(0.0f, 0.0f, 1.0f);
	arrows.back().length = 0.5f;
	arrows.back().radius = 0.025f;
	arrows.back().tipHeight = 0.25f;
	arrows.back().color = hoveredAxis == 2 ? Vec3f(1.0f, 1.0f, 0.0f) : Vec3f(1.0f, 0.0f, 0.0f);
	arrows.back().ids = Vec2u(0, 2);

	return arrows;
}

void ArrowRenderer::Draw() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBindVertexArray(vao);

	program.Bind();
	glDrawElementsInstanced(GL_TRIANGLES, TRIANGLES.size() * 3, GL_UNSIGNED_INT, 0, instanceCount);
}

// AI-generated code: Keeps the arrow gizmo at a constant screen size.
void ArrowRenderer::SetViewProjectionUniform(float data[16], float fov, int viewportHeight) {
	program.Bind();
	program.SetUniformMatrix4("viewProj", data);

	// Convert the desired on-screen height to a world-space scale at the
	// arrow's depth. The shader divides this by each arrow's local height.
	const float safeHeight = static_cast<float>(std::max(viewportHeight, 1));
	const float screenScale =
		2.0f * std::tan(degreesToRadians(fov) * 0.5f) / safeHeight * 100.0f;
	program.SetUniform("screenScale", screenScale);
}

constexpr float HALF_SQR_2 = 0.7071068f;
const std::vector<Vec3f> ArrowRenderer::VERTICES = {
	// Cylinder Base
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
	// Cylinder Top
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
	// Pyramid Base
	Vec3f(2.0, 0.0f, -2.0f), Vec3f(-2.0f, 0.0f, -2.0f), Vec3f(-2.0f, 0.0f, 2.0f),
	Vec3f(2.0f, 0.0f, 2.0f),
	// Pyramid Top
	Vec3f(0.0f, 0.0f, 0.0f)
};

const std::vector<Vec3u> ArrowRenderer::TRIANGLES = {
	// Clinder Base
	Vec3u(0, 1, 2), Vec3u(0, 2, 3), Vec3u(0, 3, 4), Vec3u(0, 4, 5), Vec3u(0, 5, 6), Vec3u(0, 6, 7),
	// Cyclinder Top
	Vec3u(8, 10, 9), Vec3u(8, 11, 10), Vec3u(8, 12, 11), Vec3u(8, 13, 12), Vec3u(8, 14, 13), Vec3u(8, 15, 14),
	// Cylinder Sides
	Vec3u(0, 8, 1), Vec3u(1, 8, 9),
	Vec3u(1, 9, 2), Vec3u(2, 9, 10),
	Vec3u(2, 10, 3), Vec3u(3, 10, 11),
	Vec3u(3, 11, 4), Vec3u(4, 11, 12),
	Vec3u(4, 12, 5), Vec3u(5, 12, 13),
	Vec3u(5, 13, 6), Vec3u(6, 13, 14),
	Vec3u(6, 14, 7), Vec3u(7, 14, 15),
	Vec3u(7, 15, 0), Vec3u(0, 15, 8),
	// Pyramid Base
	Vec3u(16, 17, 18), Vec3u(16, 17, 19),
	// Pyramid Top
	Vec3u(16, 20, 17), Vec3u(17, 20, 18), Vec3u(18, 20, 19), Vec3u(19, 20, 16)
};
