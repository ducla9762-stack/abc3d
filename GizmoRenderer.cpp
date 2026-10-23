#include <algorithm>
#include <cmath>
#include <iostream>

#include "GizmoRenderer.h"

bool GizmoRenderer::Part::Initialize(const char* vertexShader,
	const std::vector<Vec3f>& vert,
	const std::vector<Vec3u>& tri) {
	vertices = vert;
	triangles = tri;

	// Compile shaders.
	if (!visual.BuildFiles(vertexShader, "assets/shaders/GizmoColor.frag") || !pick.BuildFiles(vertexShader, "assets/shaders/GizmoPick.frag")) {
		return false;
	}

	// Upload data to buffers.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vert.size() * sizeof(Vec3f), vert.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f), nullptr);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri.size() * sizeof(Vec3u), tri.data(), GL_STATIC_DRAW);

	// Upload gizmo settings.
	glGenBuffers(1, &instances);
	glBindBuffer(GL_ARRAY_BUFFER, instances);
	const GLsizei stride = sizeof(GizmoInstanceSettings);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GizmoInstanceSettings, base));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GizmoInstanceSettings, dir));
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GizmoInstanceSettings, length));
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GizmoInstanceSettings, radius));
	glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GizmoInstanceSettings, tipSize));
	glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GizmoInstanceSettings, color));
	glVertexAttribIPointer(7, 2, GL_UNSIGNED_INT, stride, (void*)offsetof(GizmoInstanceSettings, ids));
	for (GLuint attribute = 1; attribute <= 7; ++attribute) {
		glEnableVertexAttribArray(attribute);
		glVertexAttribDivisor(attribute, 1);
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return true;
}

void GizmoRenderer::Part::Upload(const std::vector<GizmoInstanceSettings>& settings, GLenum usage) {
	glBindBuffer(GL_ARRAY_BUFFER, instances);
	glBufferData(GL_ARRAY_BUFFER, settings.size() * sizeof(GizmoInstanceSettings), settings.data(), usage);
	instanceCount = static_cast<GLsizei>(settings.size());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GizmoRenderer::Part::SetViewProjection(float data[16], float screenScale) {
	// Both passes use the same camera transform and screen-space sizing.
	visual.Bind();
	visual.SetUniformMatrix4("viewProj", data);
	visual.SetUniform("screenScale", screenScale);
	pick.Bind();
	pick.SetUniformMatrix4("viewProj", data);
	pick.SetUniform("screenScale", screenScale);
}

void GizmoRenderer::Part::SetRotation(const Matrix3f& rotation) {
	// Directional parts use this orientation to convert local X/Y/Z axes into
	// the selected object's world-space orientation.
	visual.Bind();
	visual.SetUniformMatrix3("gizmoRotation", rotation.cell);
	pick.Bind();
	pick.SetUniformMatrix3("gizmoRotation", rotation.cell);
}

void GizmoRenderer::Part::SetRingMode(float tolerance) {
	// The visible ring stays thin, while the picking ring is wider and easier
	// to hit with the mouse.
	visual.Bind();
	visual.SetUniform("ringSelectionTolerance", 1.0f);
	pick.Bind();
	pick.SetUniform("ringSelectionTolerance", tolerance);
}

void GizmoRenderer::Part::Draw(bool picking) {
	if (instanceCount == 0) { return; }

	// Select either the color shader or the ID shader, then draw every stored
	// gizmo instance with the same indexed mesh.
	glBindVertexArray(vao);
	if (picking) { pick.Bind(); }
	else { visual.Bind(); }
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(triangles.size() * 3), GL_UNSIGNED_INT, nullptr, instanceCount);
}

void GizmoRenderer::SetRotation(const Matrix3f& orientation) {
	// Keep all part programs synchronized, including the free-transform cube.
	translation.SetRotation(orientation);
	scale.SetRotation(orientation);
	this->rotation.SetRotation(orientation);
	free.SetRotation(orientation);
}

void GizmoRenderer::UploadParts() {
	// Upload the active settings for each part immediately before drawing. Empty
	// vectors naturally produce zero-instance draw calls.
	translation.Upload(arrows);
	scale.Upload(maces);
	rotation.Upload(hoops);
	free.Upload(cube);
}

bool GizmoRenderer::Initialize() {
	// Build the four reusable meshes: translation arrows, scale maces,
	// rotation rings, and the center/free-transform cube.
	if (!translation.Initialize("assets/shaders/Arrow.vert", ARROW_VERTICES, ARROW_TRIANGLES)) return false;
	if (!scale.Initialize("assets/shaders/Mace.vert", MACE_VERTICES, MACE_TRIANGLES)) return false;
	if (!rotation.Initialize("assets/shaders/RotationRing.vert", ROTATION_RING_VERTICES, ROTATION_RING_TRIANGLES)) return false;
	if (!free.Initialize("assets/shaders/Cube.vert", FREE_CUBE_VERTICES, FREE_CUBE_TRIANGLES)) return false;
	// Configure a larger tolerance for ring picking, then seed the default
	// settings. Clear() removes those settings from the active draw lists until
	// the viewport selects a gizmo type.
	rotation.SetRingMode(2.5f);
	arrows = DEFAULT_ARROW_SETTINGS;
	maces = DEFAULT_MACE_SETTINGS;
	hoops = DEFAULT_ROTATION_RING_SETTINGS;
	cube = DEFAULT_FREE_CUBE_SETTINGS;
	Clear();
	return true;
}

void GizmoRenderer::SetViewProjection(float data[16], float fov, int viewportHeight) {
	// Convert the camera FOV and viewport height into a scale that keeps gizmos
	// approximately constant in screen size as the camera moves.
	const float safeHeight = static_cast<float>(std::max(viewportHeight, 1));
	const float screenScale = 2.0f * std::tan(degreesToRadians(fov) * 0.5f) / safeHeight * 100.0f;
	translation.SetViewProjection(data, screenScale);
	scale.SetViewProjection(data, screenScale);
	rotation.SetViewProjection(data, screenScale);
	free.SetViewProjection(data, screenScale);
}

void GizmoRenderer::UpdateGizmos(const Vec3f& base, uint hoveredAxis, Type type, const Matrix3f& rotation) {
	// Select the active gizmo type and discard the other types so only one tool
	// appears at a time. The center cube is shared by all transform tools.
	SetRotation(rotation);
	std::vector<GizmoInstanceSettings>* active = nullptr;
	const std::vector<GizmoInstanceSettings>* defaults = nullptr;

	switch (type) {
	case Type::Translation: active = &arrows; defaults = &DEFAULT_ARROW_SETTINGS; maces.clear(); hoops.clear(); break;
	case Type::Scale: active = &maces; defaults = &DEFAULT_MACE_SETTINGS; arrows.clear(); hoops.clear(); break;
	case Type::Rotation: active = &hoops; defaults = &DEFAULT_ROTATION_RING_SETTINGS; arrows.clear(); maces.clear(); break;
	}
	if (cube.empty()) cube = DEFAULT_FREE_CUBE_SETTINGS;
	if (active->empty()) *active = *defaults;

	// Move each axis to the shared pivot and assign its normal or hovered color.
	for (GizmoInstanceSettings& setting : *active) {
		setting.base = base;
		const uint axis = setting.ids.y;
		setting.color = axis == hoveredAxis ? Vec3f(1.0f, 1.0f, 0.0f) :
			axis == 0 ? Vec3f(0.0f, 1.0f, 0.0f) :
			axis == 1 ? Vec3f(0.0f, 0.0f, 1.0f) :
			axis == 2 ? Vec3f(1.0f, 0.0f, 0.0f) : Vec3f(0.8f);
	}
	// The center cube uses axis ID 3 and has its own hover color state.
	for (GizmoInstanceSettings& setting : cube) {
		setting.base = base;
		setting.color = hoveredAxis == 3 ? Vec3f(1.0f, 1.0f, 0.0f) : Vec3f(0.8f);
	}
}

void GizmoRenderer::Draw() {
	// Visible pass: writes regular RGBA colors to the default framebuffer after
	// the deferred scene and grid have already been composed.
	UploadParts();
	translation.Draw(false);
	scale.Draw(false);
	rotation.Draw(false);
	free.Draw(false);
}

void GizmoRenderer::DrawPick() {
	// Picking pass: writes gizmo IDs to the off-screen integer framebuffer. It
	// has no visible color output and exists solely for cursor hit-testing.
	UploadParts();
	translation.Draw(true);
	scale.Draw(true);
	rotation.Draw(true);
	free.Draw(true);
}

void GizmoRenderer::Clear() {
	// Remove all active instances while retaining the initialized GPU resources
	// so the renderer can be repopulated on the next viewport update.
	arrows.clear();
	maces.clear();
	hoops.clear();
	cube.clear();
	translation.Clear();
	scale.Clear();
	rotation.Clear();
	free.Clear();
}
