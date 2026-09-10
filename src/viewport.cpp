#include <GL/glew.h>

#include <iostream>
#include <utility>

#include "viewport.h"
#include "mesh.h"
#include "shapes.h"
#include "render.h"
#include "scene.h"
#include "helper.h"

#include "customWindow.h"

#undef max

Fl_Window* rightClickPanel;

void Viewport::draw() {
	if (!context_valid() || modeChanged) {
		bool success = activeRenderer->Initialize();
		activeRenderer->SetCamera(sphericalToCartesian(camPos));
		modelingRenderer.UpdateRenderBuffers(scene);
		if (!success) { return; }
		modelingRenderer.UpdateSelection(selectionManager.getSelectedItems());
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

	if (Fl_Group::handle(event)) {
		return 1;
	}


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
	// Get mouse pointer position.
	Vec2f pos(Fl::event_x(), Fl::event_y());
	Vec2f uv(pos.x / (w() - 1), pos.y / (h() - 1));
	uv.y = 1.0f - uv.y;

	if (firstDragCallback) {
		initialDragDirUV = uv - mouseDragStartPosUV;
		initialDragDirUV.Normalize();
		initialDragDir = pos - mouseDragStartPos;
		initialDragDir.Normalize();
	}

	// Get change in position.
	Vec2f change;
	change.y = mouseDragPrevPos.y - pos.y;
	change.x = pos.x - mouseDragPrevPos.x;

	if (change.y == 0 && change.x == 0) return;

	bool camera = true;

	// Transformation drag depending on action.
	if (activeAction == TRANSLATE_OUT) {
		camera = !translateOutDrag(pos, uv);
	}
	else if (activeAction == TRANSLATE_FREE) {
		camera = !translateFreeDrag(pos, uv);
	}
	else if (activeAction == SCALE) {
		camera = !uniformScaleDrag(pos);
	}
	else if (activeAction == ROTATE) {
		camera = !rotateDrag(pos);
	}
	
	// Otherwise, we rotate the camera position.
	if (camera) {
		camPos.x -= change.x;
		camPos.y += change.y;
		clamp(camPos.y, 1.0f, 179.0f);
		activeRenderer->SetCamera(sphericalToCartesian(camPos));
	}

	firstDragCallback = false;
	mouseDragPrevPos = pos;
	mouseDragPrevPosUV = uv;
	redraw();
}

bool Viewport::uniformScaleDrag(const Vec2f& pos) {
	if (activeAction != SCALE || selectionIds.x == 0) {
		return false;
	}

	Vec2f change = pos - mouseDragPrevPos;
	float factor = 0.01f * change.Dot(initialDragDir);

	uint objectId = selectionIds.x != MAX_UINT ? selectionIds.x : selectionIds.y;
	scene[objectId - 1]->scale += factor;
	modelingRenderer.UpdateRenderBuffers(scene);

	if (primarySelectionCallback) { primarySelectionCallback(); }

	return true;
}

bool Viewport::translateOutDrag(const Vec2f& pos, const Vec2f& uv) {
	if (selectionIds.x == 0) { return false; }

	if (activeElement == OBJECT) {
		return translateFreeDrag(pos, uv);
	}

	Vec2f change = pos - mouseDragPrevPos;
	float scale = 0.01f * change.Dot(initialDragDir);

	if (activeElement == VERTEX) {
		return translateOutVertexDrag(scale);
	}
	else if (selectionIds.x == MAX_UINT) {
		return false;
	}

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

	if (primarySelectionCallback) { primarySelectionCallback(); }
	modelingRenderer.UpdateRenderBuffers(scene);

	return true;
}

bool Viewport::translateOutVertexDrag(float scale) {
	if (selectionIds.x != MAX_UINT) {
		return false;
	}

	if (firstDragCallback) {
		actionDir = scene[selectionIds.y - 1]->GetVertexNormal(selectionIds.z);
		//initialActionState = scene[selectionIds.y - 1]->GetVertexPosition(selectionIds.z);
	}

	scene[selectionIds.y - 1]->TranslateVertex(selectionIds.z, scale * actionDir);
	modelingRenderer.UpdateRenderBuffers(scene);

	if (primarySelectionCallback) { primarySelectionCallback(); }

	return true;
}

bool Viewport::translateFreeDrag(const Vec2f& pos, const Vec2f& uv) {
	Vec3f f, u, r;
	modelingRenderer.GetCameraDirs(f, u, r);

	Vec2f change;
	change.x = mouseDragPrevPos.x - pos.x;
	change.y = pos.y - mouseDragPrevPos.y;
	Vec3f translation = 0.01f * (change.x * r + change.y * u);

	if (activeElement == VERTEX) {
		if (selectionIds.x != MAX_UINT) {
			return false;
		}

		translation = scene[selectionIds.y - 1]->WorldDirToLocalDir(translation);
		scene[selectionIds.y - 1]->TranslateVertex(selectionIds.z, translation);
		modelingRenderer.UpdateRenderBuffers(scene);
		return true;
	}

	if (selectionIds.x == 0 || selectionIds.x == MAX_UINT) {
		return false;
	}

	switch (activeElement) {
	case OBJECT:
		scene[selectionIds.x - 1]->pos += translation;
		
		break;
	case EDGE:
		translation = scene[selectionIds.x - 1]->WorldDirToLocalDir(translation);
		scene[selectionIds.x - 1]->TranslateEdge(selectionIds.z, translation);
		break;
	case FACE:
		translation = scene[selectionIds.x - 1]->WorldDirToLocalDir(translation);
		scene[selectionIds.x - 1]->TranslateFace(selectionIds.y, translation);
		break;
	}

	modelingRenderer.UpdateRenderBuffers(scene);

	if (primarySelectionCallback) { primarySelectionCallback(); }

	return true;
}

bool Viewport::rotateDrag(const Vec2f& pos) {
	if (selectionIds.x == 0 || selectionIds.x == MAX_UINT) {
		return false;
	}

	Vec3f f, u, r;
	modelingRenderer.GetCameraDirs(f, u, r);

	Vec2f change;
	change.y = mouseDragPrevPos.x - pos.x;
	change.x = -(pos.y - mouseDragPrevPos.y);

	Vec3f dir = change.x * r + change.y * u;
	float length = dir.Length();
	dir /= length;

	Vec3f eulerRot = scene[selectionIds.x - 1]->eulerRot;
	Matrix3f origRot;
	origRot.SetRotationZYX(degreesToRadians(eulerRot.x), degreesToRadians(eulerRot.y), degreesToRadians(eulerRot.z));

	Matrix3f rot;
	rot.SetRotation(dir, length * 0.01f);
	rot *= origRot;

	scene[selectionIds.x - 1]->eulerRot = matrixToEulerAngle(rot);

	modelingRenderer.UpdateRenderBuffers(scene);

	if (primarySelectionCallback) { primarySelectionCallback(); }

	return true;
}

int Viewport::mousePush() {
	bool panelShown = false;
	// Get position of mouse press.
	uint x = (float(Fl::event_x()) / w()) * pixel_w();
	uint y = (1.0f - (float(Fl::event_y()) / h())) * pixel_h();


	if (Fl::event_button() == FL_RIGHT_MOUSE) {
		panelShown = true;
		std::cout << panelShown << std::endl;
		rightClickPanel->position(Fl::event_x(), Fl::event_y());
		rightClickPanel->show();
		return 1;
	}

	else if (Fl::event_button() == FL_LEFT_MOUSE) {
		std::cout << panelShown << std::endl;
		if (panelShown == true) {
			rightClickPanel->hide();
			panelShown = false;
		}

		// Get the selection ids at this position.
		selectionIds = modelingRenderer.SelectionIds(x, y);

		// Create a selection item to send to the selection manager.
		std::optional<SelectionItem> item = createSelectionItem(selectionIds);

		// query the selection maanger with the item.
		selectionManager.processSelection(item, additiveSelection);

		// Tell the renderer to update the selection visuals.
		modelingRenderer.UpdateSelection(selectionManager.getSelectedItems());

		if (primarySelectionCallback) {
			primarySelectionCallback();
		}

		redraw();

		// Print selection ids.
		std::cout << "Object Id: " << selectionIds.x << std::endl;
		std::cout << "Component Id: " << selectionIds.y << std::endl;
		std::cout << "Sub-Component Id: " << selectionIds.z << std::endl;

		// Drag stuff, refactor later.
		mouseDragStartPos.Set(Fl::event_x(), Fl::event_y());
		mouseDragPrevPos = mouseDragStartPos;
		mouseDragStartPosUV.Set(mouseDragStartPos.x / (w() - 1), mouseDragStartPos.y / (h() - 1));
		mouseDragStartPosUV.y = 1.0f - mouseDragStartPosUV.y;
		mouseDragPrevPosUV = mouseDragStartPosUV;
		firstDragCallback = true;
		//redraw();
	}
}

void Viewport::mouseWheel() {
	// Adjust the camera zoom.
	camPos.z += Fl::event_dy();
	if (camPos.z < 0.0f) { camPos.z = 0.0f; }

	camPos.x += Fl::event_dx();

	activeRenderer->SetCamera(sphericalToCartesian(camPos));
	redraw();
}

std::optional<SelectionItem> Viewport::createSelectionItem(Vec3u ids)
{
	switch (selectionManager.getActiveElement()) {
	case OBJECT:
		// Normal hit.
		if (ids.x != 0 && ids.x != MAX_UINT) {
			return SelectionItem{
				OBJECT,
				ids.x - 1,
				MAX_UINT
			};
		}
		// Vertex hit belonging to object.
		if (ids.x == MAX_UINT && ids.y != 0) {
			return SelectionItem{
				OBJECT,
				ids.y - 1,
				MAX_UINT
			};
		}

		break;

	case VERTEX:
		if (ids.x == MAX_UINT && ids.y != 0) {
			return SelectionItem{
				VERTEX,
				ids.y - 1,
				ids.z
			};
		}

		break;

	case FACE:
		if (ids.x != 0 && ids.x != MAX_UINT) {
			return SelectionItem{
				FACE,
				ids.x - 1,
				ids.y
			};
		}

		break;

	case EDGE:
		if (ids.x != 0 && ids.x != MAX_UINT && ids.z != MAX_UINT) {
			return SelectionItem{
				EDGE,
				ids.x - 1,
				ids.z
			};
		}

		break;
	}

	// Null if background hit.
	return std::nullopt;
}

Viewport::Viewport(int x, int y, int w, int h, const char* label)
	: Fl_Gl_Window(x, y, w, h, label)
{
	Fl_Gl_Window::mode(FL_RGB8 | FL_ALPHA | FL_DEPTH | FL_DOUBLE | FL_OPENGL3);
	// Load default mesh (cube).

	rightClickPanel = new Fl_Window(0, 0, 40, 60, 0);
	rightClickPanel->color(6);
	rightClickPanel->hide();

	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);
}

float Viewport::getScale(int component, float scale) {
	// todo
	return 1;
}

void Viewport::setScale(int component, float scale) {
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];

		switch (item.type) {
		case OBJECT:
			mesh->scale[component] = scale;
			break;

		case VERTEX:
			// todo
			break;

		case EDGE:
			// todo
			break;

		case FACE:
			// todo
			break;
		}
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::bevel() {
	// todo
}

void Viewport::extrude() {
	// todo
}

void Viewport::subdivision() {
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];

		switch (item.type) {
		case OBJECT:
			mesh->subdivide();
			break;

		case VERTEX:
			// todo
			break;

		case EDGE:
			// todo
			break;

		case FACE:
			// todo
			break;
		}
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayCube() {
	// Insert new cube mesh into scene.
	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayPyramid() {
	// Insert new pyramid mesh into scene.
	scene.NewMesh()->load(PYRAMID_VERTICES, PYRAMID_FACES);
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayCylinder() {
	std::vector<Vec3f> vertices;
	std::vector<Vec4u> faces;

	// Generate cylinder of resolution 12.
	generateUnitCylinder(vertices, faces, 12);
	// Insert new cylinder mesh into scene.
	scene.NewMesh()->load(vertices, faces);

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayIcoSphere() {
	// Insert new icosphere mesh into scene.
	scene.NewMesh()->load(ICOSPHERE_VERTICES, ICOSPHERE_FACES);
	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayUVSphere() {
	std::vector<Vec3f> vertices;
	std::vector<Vec4u> faces;

	// Generate uv sphere with longitude and lattitude of 10.
	generateUVSphere(vertices, faces, 10, 10);
	// Insert new uv sphere mesh into scene.
	scene.NewMesh()->load(vertices, faces);

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::displayTorus() {
	std::vector<Vec3f> vertices;
	std::vector<Vec4u> faces;

	// Generate torus.
	generateUnitTorus(vertices, faces, 40, 20, 1.0f, 0.3f);
	// Insert new torus mesh into scene.
	scene.NewMesh()->load(vertices, faces);

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}


void Viewport::tesselate() {
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];

		switch (item.type) {
		case OBJECT:
			mesh->tesselate();
			break;

		case VERTEX:
			// todo
			break;

		case EDGE:
			// todo
			break;

		case FACE:
			// todo
			break;
		}
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

void Viewport::deleteElement() {
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];

		switch (item.type) {
		case OBJECT:
			scene.DeleteMesh(item.meshId);
			break;

		case VERTEX:
			// todo
			break;

		case EDGE:
			// todo
			break;

		case FACE:
			// todo
			break;
		}
	}

	selectionManager.clearSelection();
	modelingRenderer.UpdateRenderBuffers(scene);
	primarySelection = MAX_UINT;
	if (primarySelectionCallback) { primarySelectionCallback(); }
	redraw();
}

void Viewport::duplicateElement() {
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];

		switch (item.type) {
		case OBJECT:
			scene.DuplicateMesh(item.meshId);
			break;

		case VERTEX:
			// todo
			break;

		case EDGE:
			// todo
			break;

		case FACE:
			// todo
			break;
		}
	}
}

float Viewport::getRot(int component, float angle) {
	// todo
	return 1;
}

void Viewport::setRot(int component, float angle) {
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];

		switch (item.type) {
		case OBJECT:
			mesh->eulerRot[component] = angle;
			break;

		case VERTEX:
			// todo
			break;

		case EDGE:
			// todo
			break;

		case FACE:
			// todo
			break;
		}
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

float Viewport::getPos(int component, float angle) {
	// todo
	return 1;
}

void Viewport::setPos(int component, float pos) {
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];

		switch (item.type) {
		case OBJECT:
			mesh->pos[component] = pos;
			break;

		case VERTEX:
			// todo
			break;

		case EDGE:
			// todo
			break;

		case FACE:
			// todo
			break;
		}
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
}

ImportReport Viewport::loadFile(const char* filepath) {
	// Build into local meshes first so a rejected file leaves the scene untouched.
	// Adding to the scene up front would strand empty meshes on every failed import.
	std::vector<HalfEdgeMesh> candidates;
	ImportReport report = loadMeshes(filepath, candidates);
	if (!report.ok()) { return report; }

	for (HalfEdgeMesh& candidate : candidates)
		*scene.NewMesh() = std::move(candidate);

	modelingRenderer.UpdateRenderBuffers(scene);
	redraw();
	return report;
}


std::vector<uint> Viewport::exportMeshIds() {
	std::vector<uint> selected; 
	std::vector<uint> all; 

	for (uint i = 0; i < scene.MeshCount(); ++i) {
		if (scene[i]->VertexCount() == 0) continue;

		all.push_back(i); 
		if (selectionManager.hasSelectedMesh(i)) {
			selected.push_back(i);
		}
	}

	return selected.empty() ? all : selected; 
}

bool Viewport::exportObj(const char* filepath) {
	std::vector<uint> ids = exportMeshIds(); 
	if (ids.empty()) return true; 

	std::ofstream file(ensureExtension(filepath, ".obj")); 
	if (!file.is_open()) return false; 

	uint vertOffset = 0; 

	for (uint id : ids) {
		file << "o mesh" << id << "\n"; 
		scene[id]->appendObj(file, vertOffset); 
	}

	return !file.fail(); 
}

bool Viewport::exportStl(const char* filepath) {
	std::vector<uint> ids = exportMeshIds();
	if (ids.empty()) return true;

	std::vector<Triangle> tris; 
	for (uint id : ids)
		scene[id]->appendStlTriangles(tris); 
	
	return writeStl(ensureExtension(filepath, ".stl"), tris, true); 
}

Mode Viewport::getActiveMode() { return activeMode;  }

Element Viewport::getActiveElement() { 
	//return activeElement;
	return selectionManager.getActiveElement();
}

Action Viewport::getActiveAction() { return activeAction; }

void Viewport::setActiveMode(Mode mode) { activeMode = mode; }

void Viewport::setActiveElement(Element element) { 
	activeElement = element;
	selectionManager.setActiveElement(element);
	modelingRenderer.UpdateSelection(selectionManager.getSelectedItems());
	if (primarySelectionCallback) {
		primarySelectionCallback();
	}
	redraw();
}

void Viewport::setActiveAction(Action action) { activeAction = action; }

const HalfEdgeMesh* Viewport::primarySelectedItem() {

	// Check if there are no selected items.
	if (selectionManager.getSelectedItems().empty())
	{
		return nullptr;
	}

	// If there are meshes, get the last selected one and return it to the UI.
	if (!scene.Meshes().empty()) {
		SelectionItem lastSelection = selectionManager.getSelectedItems().back();
		return scene[lastSelection.meshId];
	}
	// Else there are no meshes.
	else {
		return nullptr;
	}
}

void Viewport::setPrimarySelectionCallback(void (*callback)()) {
	primarySelectionCallback = callback;
}

void Viewport::togglePoints() {
	modelingRenderer.ToggleVertexPoints();
	redraw();
}

void Viewport::toggleGrid() {
	modelingRenderer.ToggleGrid();
	redraw();
}

void Viewport::enableAdditiveSelection() {
	additiveSelection = true;
}

void Viewport::disableAdditiveSelection() {
	additiveSelection = false;
}

