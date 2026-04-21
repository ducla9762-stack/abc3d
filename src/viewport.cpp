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
	Vec2f pos(Fl::event_x(), Fl::event_y());
	Vec2f uv(pos.x / (w() - 1), pos.y / (h() - 1));
	uv.y = 1.0f - uv.y;

	if (firstDragCallback) {
		initialDragDir = uv - mouseDragStartPosUV;
		initialDragDir.Normalize();
	}

	Vec2f change;
	change.y = mouseDragPrevPos.y - pos.y;
	change.x = pos.x - mouseDragPrevPos.x;

	if (change.y == 0 && change.x == 0) return;

	bool camera = true;
	if (activeAction == TRANSLATE_OUT) {
		camera = !translateOutDrag(pos, uv);
	}
	else if (activeAction == TRANSLATE_FREE) {
		camera = !translateFreeDrag(pos, uv);
	}
	
	if (camera) {
		camPos.x -= change.x;
		camPos.y += change.y;
		clamp(camPos.y, 1.0f, 179.0f);
		activeRenderer->SetCamera(sphericalToCartesian(camPos));
	}

	firstDragCallback = false;
	mouseDragPrevPos.Set(Fl::event_x(), Fl::event_y());
	mouseDragPrevPosUV = uv;
	redraw();
}

bool Viewport::translateOutDrag(const Vec2f& pos, const Vec2f& uv) {

	if (activeElement == OBJECT || selectionIds.x == 0) {
		return false;
	}

	if (activeElement == VERTEX) {
		return translateOutVertexDrag(uv);
	}
	else if (selectionIds.x == MAX_UINT) {
		return false;
	}

	Vec2f changeUV = uv - mouseDragPrevPosUV;
	float scale = changeUV.Length();
	changeUV /= scale;
	scale = scale * changeUV.Dot(initialDragDir);

	if (activeElement == FACE) {
		if (firstDragCallback) {
			actionDir = scene[selectionIds.x - 1]->GetFaceNormal(selectionIds.y);
		}

		scene[selectionIds.x - 1]->TranslateFace(selectionIds.y, actionDir * scale);
	}
	else if (activeElement == EDGE) {
		if (firstDragCallback) {
			actionDir = scene[selectionIds.x - 1]->GetEdgeNormal(selectionIds.z);
		}

		scene[selectionIds.x - 1]->TranslateEdge(selectionIds.z, actionDir * scale);
	}

	modelingRenderer.UpdateRenderBuffers(scene);

	return true;
}

bool Viewport::translateOutVertexDrag(const Vec2f& uv) {
	if (selectionIds.x != MAX_UINT) {
		return false;
	}

	if (firstDragCallback) {
		actionDir = scene[selectionIds.y - 1]->GetVertexNormal(selectionIds.z);
		initialActionState = scene[selectionIds.y - 1]->GetVertexPosition(selectionIds.z);
	}

	float scale = 2.0f * (uv - mouseDragStartPosUV).Length();

	scene[selectionIds.y - 1]->SetVertexPosition(selectionIds.z, initialActionState + scale * actionDir);
	modelingRenderer.UpdateRenderBuffers(scene);
	return true;
}

bool Viewport::translateFreeDrag(const Vec2f& pos, const Vec2f& uv) {
	if (activeElement == FACE || activeElement == EDGE) {
		return false;
	}

	Vec3f f, u, r;
	modelingRenderer.GetCameraDirs(f, u, r);

	Vec2f change = mouseDragPrevPosUV - uv;
	Vec3f translation = 4.0f * (change.x * r + change.y * u);

	if (activeElement == VERTEX) {
		if (selectionIds.x != MAX_UINT) {
			return false;
		}

		translation = scene[selectionIds.y - 1]->WorldDirToLocalDir(translation);
		scene[selectionIds.y - 1]->TranslateVertex(selectionIds.z, translation);
	}
	else if (activeElement == OBJECT) {
		if (selectionIds.x == 0 || selectionIds.x == MAX_UINT) {
			return false;
		}
		scene[selectionIds.x - 1]->pos += translation;
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	return true;
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

	mouseDragStartPos.Set(Fl::event_x(), Fl::event_y());
	mouseDragPrevPos = mouseDragStartPos;
	mouseDragStartPosUV.Set(float(Fl::event_x()) / (w() - 1), float(Fl::event_y()) / (h() - 1));
	mouseDragStartPosUV.y = 1.0f - mouseDragStartPosUV.y;
	mouseDragPrevPosUV = mouseDragStartPosUV;
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
		activeElement = EDGE;
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

Viewport::Viewport(int x, int y, int w, int h, const char* label)
	: Fl_Gl_Window(x, y, w, h, label)
{
	Fl_Gl_Window::mode(FL_RGB8 | FL_ALPHA | FL_DEPTH | FL_DOUBLE | FL_OPENGL3);
	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);
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

void Viewport::displayCylinder() {
	std::vector<Vec3f> vertices;
	std::vector<Vec4u> faces;

	generateUnitCylinder(vertices, faces, 12);
	scene.NewMesh()->load(vertices, faces);

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayIcoSphere() {
	scene.NewMesh()->load(ICOSPHERE_VERTICES, ICOSPHERE_FACES);
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayUVSphere() {
	std::vector<Vec3f> vertices;
	std::vector<Vec4u> faces;

	generateUVSphere(vertices, faces, 10, 10);
	scene.NewMesh()->load(vertices, faces);

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayTorus() {
	std::vector<Vec3f> vertices;
	std::vector<Vec4u> faces;

	generateUnitTorus(vertices, faces, 40, 20, 1.0f, 0.3f);
	scene.NewMesh()->load(vertices, faces);

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
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

	modelingRenderer.UpdateSelectionTexture(scene.SelectedMeshes());
	redraw();
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
	modelingRenderer.UpdateSelectionTexture(scene.SelectedMeshes());
	redraw();
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