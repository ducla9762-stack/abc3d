#include <GL/glew.h>

#include <iostream>

#include "helper.h"
#include "mesh.h"
#include "shapes.h"
#include "render.h"
#include "scene.h"
#include "viewport.h"

void Viewport::draw() {
	if (!context_valid() || modeChanged) {
		bool success = activeRenderer->Initialize();
		activeRenderer->SetCamera(sphericalToCartesian(camPos));
		modelingRenderer.UpdateRenderBuffers(scene);
		if (!success) { return; }
		modeChanged = false;
	}
	if (!valid()) {
		activeRenderer->Resize(pixel_w(), pixel_h());
	}

	activeRenderer->Draw();
}

int Viewport::handle(int event) {
	// Make sure Glew is initialized. This comes from FLTK's manual (modified).
	if (initGlew && event == FL_SHOW && shown()) {
		initGlew = false;
		make_current();
		glewInit();
	}

	if (context() == nullptr) { return 0; }

	make_current();
	// Process event
	switch (event) {
	case FL_PUSH:
		take_focus();
		return mousePush();
	case FL_DRAG:
		mouseDrag();
		break;
	case FL_MOUSEWHEEL:
		mouseWheel();
		break;
	case FL_KEYDOWN:
		keyDown();
		break;
	case FL_FOCUS:
	case FL_UNFOCUS:
		break;
	default:
		return 0; // Tell FLTK this widget doesn't want the event
	}

	// For debugging
	std::cout << "Window Event: " << fl_eventnames[event] << std::endl;

	return 1;
}

void Viewport::mouseDrag() {
	float x = Fl::event_x();
	float y = Fl::event_y();

	Vec2f change;
	change.y = mouseDragPrevPos.y - y;
	change.x = x - mouseDragPrevPos.x;

	if (change.y == 0 && change.x == 0) return;

	camPos.x -= change.x;
	camPos.y += change.y;
	clamp(camPos.y, 1.0f, 179.0f);
	activeRenderer->SetCamera(sphericalToCartesian(camPos));

	mouseDragPrevPos.Set(x, y);
	redraw();
}

int Viewport::mousePush() {
	if (Fl::event_button() != FL_LEFT_MOUSE) return 0;

	uint x = (float(Fl::event_x()) / w()) * pixel_w();
	uint y = (1.0f - (float(Fl::event_y()) / h())) * pixel_h();

	Vec3u id = modelingRenderer.SelectionIds(x, y);

	std::cout << "Object Id: " << id.x << std::endl;
	std::cout << "Component Id: " << id.y << std::endl;
	std::cout << "Sub-Component Id: " << id.z << std::endl;

	mouseDragPrevPos.Set(Fl::event_x(), Fl::event_y());
	//redraw();
}

void Viewport::mouseWheel() {
	camPos.z += Fl::event_dy();
	if (camPos.z < 0.0f) { camPos.z = 0.0f; }

	camPos.x += Fl::event_dx();

	activeRenderer->SetCamera(sphericalToCartesian(camPos));
	redraw();
}

void Viewport::keyDown() {
	switch (Fl::event_key()) {
	case 's':
		for (HalfEdgeMesh* mesh : scene.Meshes())
		{
			mesh->subdivide();
		}
		std::cout << "subdivided underlying mesh" << std::endl;
		modelingRenderer.UpdateRenderBuffers(scene);
		break;
	case 't':
		for (HalfEdgeMesh* mesh : scene.Meshes())
		{
			mesh->tesselate();
		}
		std::cout << "tesselated underlying mesh" << std::endl;
		modelingRenderer.UpdateRenderBuffers(scene);
		break;
	case 'p':
		modelingRenderer.ToggleVertexPoints();
		break;
	case 'g':
		modelingRenderer.ToggleGrid();
		break;
	case FL_Delete:
		std::cout << "mesh deleted" << std::endl;
		break;
	default:
		return;
	}
	redraw();
}

//void setMeshSelectionBoolean(bool selected)
//{
//	meshProgram.Bind();
//	meshProgram.SetUniform("selected", selected);
//}

//void initMeshModelSelectionBoolean() {
//	meshProgram.Bind();
//	meshProgram.SetUniform("selected", false);
//}

Viewport::Viewport(int x, int y, int w, int h, const char* label)
	: Fl_Gl_Window(x, y, w, h, label)
{
	Fl_Gl_Window::mode(FL_RGB8 | FL_ALPHA | FL_DEPTH | FL_DOUBLE | FL_OPENGL3);
	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);
}

void Viewport::setScale(int component, float scale) {
	for (HalfEdgeMesh* mesh : scene.Meshes())
	{
		mesh->scale[component] = scale;
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::subdivision() {
	for (HalfEdgeMesh* mesh : scene.Meshes())
	{
		mesh->subdivide();
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::tesselate() {
	for (HalfEdgeMesh* mesh : scene.Meshes())
	{
		mesh->tesselate();
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::setRot(int component, float angle) {
	for (HalfEdgeMesh* mesh : scene.Meshes())
	{
		mesh->eulerRot[component] = angle;
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::setPos(int component, float pos) {
	for (HalfEdgeMesh* mesh : scene.Meshes())
	{
		mesh->pos[component] = pos;
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

bool Viewport::loadFile(const char* filepath) {
	bool success = scene.NewMesh()->load(filepath);
	if (!success) return false;
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
	return true;
}

// TODO: Fix export
bool Viewport::exportObj(const char* filepath) {
	if (scene.MeshCount() == 0) {
		return true;
	}

	scene[0]->exportObj(filepath);
	return true;
}

bool Viewport::exportStl(const char* filepath) {
	if (scene.MeshCount() == 0) {
		return true;
	}

	scene[0]->exportStl(filepath);
	return true;
}