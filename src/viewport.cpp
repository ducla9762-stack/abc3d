#include <GL/glew.h>

#include <iostream>

#include "viewport.h"
#include "mesh.h"
#include "shapes.h"
#include "render.h"
#include "scene.h"
#include "helper.h"

#undef max

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
	case FL_KEYUP:
		keyUp();
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

	if (activeElement == VERTEX && activeAction == TRANSLATE_OUT && selectionIds.x == MAX_UINT) {
		if (firstDragCallback) {
			actionDir = scene[selectionIds.y - 1]->GetVertexNormal(selectionIds.z);
			initialActionState = scene[selectionIds.y - 1]->GetVertexPosition(selectionIds.z);
		}
		Vec2f uv;
		uv[0] = float(x) / float(pixel_w());
		uv[1] = float(y) / float(pixel_h());
		float scale = 2.0f * (uv - mouseDragStartPos).Length();

		scene[selectionIds.y - 1]->SetVertexPosition(selectionIds.z, initialActionState + scale * actionDir);
		modelingRenderer.UpdateRenderBuffers(scene);
	}
	else {
		camPos.x -= change.x;
		camPos.y += change.y;
		clamp(camPos.y, 1.0f, 179.0f);
		activeRenderer->SetCamera(sphericalToCartesian(camPos));
	}

	firstDragCallback = false;
	mouseDragPrevPos.Set(x, y);
	redraw();
}

int Viewport::mousePush() {
	if (Fl::event_button() != FL_LEFT_MOUSE) return 0;

	uint x = (float(Fl::event_x()) / w()) * pixel_w();
	uint y = (1.0f - (float(Fl::event_y()) / h())) * pixel_h();

	selectionIds = modelingRenderer.SelectionIds(x, y);

	// Check if there is a valid object-level id.
	if (selectionIds.x != 0 && selectionIds.x != MAX_UINT) {
		// Use the 0 indexed id.
		selectMesh(selectionIds.x - 1);
	}
	else {
		deselectMeshes();
	}

	std::cout << "Object Id: " << selectionIds.x << std::endl;
	std::cout << "Component Id: " << selectionIds.y << std::endl;
	std::cout << "Sub-Component Id: " << selectionIds.z << std::endl;

	mouseDragStartPos.Set(float(Fl::event_x()) / pixel_w(), float(Fl::event_y()) / pixel_h());
	mouseDragPrevPos.Set(Fl::event_x(), Fl::event_y());
	firstDragCallback = true;
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
		subdivision();
		break;
	case 't':
		tesselate();
		break;
	case 'p':
		modelingRenderer.ToggleVertexPoints();
		break;
	case 'g':
		modelingRenderer.ToggleGrid();
		break;
	case FL_Delete:
		deleteMesh();
		break;
	case 'd': 
		duplicateMesh(); 
		break;
	case 'v':
		activeElement = VERTEX;
		activeAction = TRANSLATE_OUT;
		break;
	case FL_Control_L:
		singleSelection = false;
		break;
	default:
		return;
	}
	redraw();
}

void Viewport::keyUp() {
	switch (Fl::event_key()) {
	case FL_Control_L:
		singleSelection = true;
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
	//scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);
	//scene.NewMesh()->load(PYRAMID_VERTICES, PYRAMID_FACES);
	std::vector<Vec3f> vertices;
	std::vector<Vec4u> faces; 

	generateUnitTorus(vertices, faces, 40, 20, 1.0f, 0.3f); 
	scene.NewMesh()->load(vertices, faces);
}

float Viewport::getScale(int component, float scale) {
	return 1;
}

void Viewport::setScale(int component, float scale) {
	//for (HalfEdgeMesh* mesh : scene.Meshes())
	//{
	//	mesh->scale[component] = scale;
	//}
	//modelingRenderer.UpdateRenderBuffers(scene);
	//redraw();

	for (uint i = 0; i < scene.SelectedMeshes().size(); i++)
	{
		if (scene.IsMeshSelected(i))
		{
			scene.Meshes().at(i)->scale[component] = scale;
		}
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::bevel() {

}

void Viewport::extrude() {

}

void Viewport::subdivision() {
	//for (HalfEdgeMesh* mesh : scene.Meshes())
	//{
	//	mesh->subdivide();
	//}
	//modelingRenderer.UpdateRenderBuffers(scene);
	//redraw();

	for (uint i = 0; i < scene.SelectedMeshes().size(); i++)
	{
		if (scene.IsMeshSelected(i))
		{
			scene.Meshes().at(i)->subdivide();
		}
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayCube() {
	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayPyramid() {
	scene.NewMesh()->load(PYRAMID_VERTICES, PYRAMID_FACES);
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displaySphere() {
	//scene.NewMesh()->load()
}

void Viewport::displayTorus() {

}


void Viewport::tesselate() {
	//for (HalfEdgeMesh* mesh : scene.Meshes())
	//{
	//	mesh->tesselate();
	//}
	//modelingRenderer.UpdateRenderBuffers(scene);
	//redraw();

	for (uint i = 0; i < scene.SelectedMeshes().size(); i++)
	{
		if (scene.IsMeshSelected(i))
		{
			scene.Meshes().at(i)->tesselate();
		}
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::deleteMesh() {
	//for (uint i = 0; i < scene.MeshCount(); i++)
	//{
	//	scene.DeleteMesh(i);
	//}
	//std::cout << "mesh deleted" << std::endl;
	//modelingRenderer.UpdateRenderBuffers(scene);

	for (uint i = 0; i < scene.SelectedMeshes().size(); i++)
	{
		if (scene.IsMeshSelected(i))
		{
			scene.DeleteMesh(i);
		}
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::selectMesh(uint id) {
	// If single selection, remove all other selections before adding to selection.
	if (singleSelection) {
		for (uint i = 0; i < scene.SelectedMeshes().size(); i++) {
			scene.RemoveFromSelectedMeshes(i);
		}
	}

	// If not selected, select.
	if (!scene.IsMeshSelected(id)) {
		scene.AddToSelectedMeshes(id);
	}
	// If selected, remove selection.
	else {
		scene.RemoveFromSelectedMeshes(id);
	}
}

void Viewport::duplicateMesh() {
	
	for (uint i = 0; i < scene.SelectedMeshes().size(); i++) 
	{
		if (scene.IsMeshSelected(i)) {
			scene.DuplicateMesh(i);
			std::cout << "This should duplicate" << std::endl;
		}
			
	}
}


void Viewport::deselectMeshes() {
	for (uint i = 0; i < scene.SelectedMeshes().size(); i++) {
		scene.RemoveFromSelectedMeshes(i);
	}
}

float Viewport::getRot(int component, float angle) {
	return 1;
}

void Viewport::setRot(int component, float angle) {
	//for (HalfEdgeMesh* mesh : scene.Meshes())
	//{
	//	mesh->eulerRot[component] = angle;
	//}
	//modelingRenderer.UpdateRenderBuffers(scene);
	//redraw();

	for (uint i = 0; i < scene.SelectedMeshes().size(); i++)
	{
		if (scene.IsMeshSelected(i))
		{
			scene.Meshes().at(i)->eulerRot[component] = angle;
		}
	}
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

float Viewport::getPos(int component, float angle) {
	return 1;
}

void Viewport::setPos(int component, float pos) {
	//for (HalfEdgeMesh* mesh : scene.Meshes())
	//{
	//	mesh->pos[component] = pos;
	//}
	//modelingRenderer.UpdateRenderBuffers(scene);
	//redraw();

	for (uint i = 0; i < scene.SelectedMeshes().size(); i++)
	{
		if (scene.IsMeshSelected(i))
		{
			scene.Meshes().at(i)->pos[component] = pos;
		}
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

Mode Viewport::getActiveMode() { return activeMode;  }

Element Viewport::getActiveElement() { return activeElement; }

Action Viewport::getActiveAction() { return activeAction; }

void Viewport::setActiveMode(Mode mode) { activeMode = mode; }

void Viewport::setActiveElement(Element element) { activeElement = element; }

void Viewport::setActiveAction(Action action) { activeAction = action; }