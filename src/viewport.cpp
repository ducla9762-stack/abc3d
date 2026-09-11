#include <GL/glew.h>

#include <iostream>
#include <unordered_set>
#include <utility>

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
		modelingRenderer.UpdateSelection(selectionManager.getSelectedItems());
		modelingRenderer.UpdateHover(hoveredSelectionIds, getActiveElement());
		updateArrows();
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

	// AI-generated code: Routes viewport hover events before Fl_Group handling.
	// FLTK's group handling can consume motion events before they reach the
	// event switch below, so handle viewport hover events first.
	if (event == FL_MOVE) {
		make_current();
		mouseMove();
		return 1;
	}
	if (event == FL_LEAVE) {
		make_current();
		mouseLeave();
		return 1;
	}

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

// AI-generated code: Clears mesh-element and arrow hover state on exit.
void Viewport::mouseLeave() {
	// Check if we are hovering over something new.
	const bool hoverChanged =
		hoveredArrowAxis != MAX_UINT ||
		hoveredSelectionIds.x != MAX_UINT ||
		hoveredSelectionIds.y != MAX_UINT ||
		hoveredSelectionIds.z != MAX_UINT;
	// Don't do anything if nothing has changed.
	if (!hoverChanged) {
		return;
	}

	// Set hover selection to be invalid.
	hoveredArrowAxis = MAX_UINT;
	hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	// Update renderer, redraw.
	modelingRenderer.UpdateHover(hoveredSelectionIds, getActiveElement());
	updateArrows();
	redraw();
}

// AI-generated code: Reads the hovered framebuffer ID and updates highlights.
void Viewport::mouseMove() {
	if (w() <= 0 || h() <= 0 || pixel_w() <= 0 || pixel_h() <= 0) {
		return;
	}

	uint x = static_cast<uint>((float(Fl::event_x()) / w()) * pixel_w());
	uint y = static_cast<uint>((1.0f - (float(Fl::event_y()) / h())) * pixel_h());
	x = x >= static_cast<uint>(pixel_w()) ? pixel_w() - 1 : x;
	y = y >= static_cast<uint>(pixel_h()) ? pixel_h() - 1 : y;

	// Get what is at the mouse pointer.
	const Vec3u ids = modelingRenderer.SelectionIds(x, y);
	// Get the axis of the arrow we are hovering over.
	const uint newHoveredAxis =
		// Only if we are in translate axis mode, and touching a valid arrow.
		(activeAction == TRANSLATE_AXIS && ids.x == ARROW_IDENTIFIER && ids.y == 0 && ids.z < 3)
		// If true, get the axis component of the arrow id.
		? ids.z
		// Else not valid, give max uint.
		: MAX_UINT;

	// Check if we changed hover.
	const bool idsChanged = ids.x != hoveredSelectionIds.x || ids.y != hoveredSelectionIds.y || ids.z != hoveredSelectionIds.z;
	// Check if we are hovering over a new arrow.
	const bool arrowChanged = newHoveredAxis != hoveredArrowAxis;

	hoveredSelectionIds = ids;
	// Update hover in renderer.
	modelingRenderer.UpdateHover(hoveredSelectionIds, getActiveElement());
	// If no change in hover, do nothing.
	if (!idsChanged && !arrowChanged) {
		return;
	}

	// Else, update.
	hoveredArrowAxis = newHoveredAxis;
	if (arrowChanged) {
		updateArrows();
	}
	redraw();
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

	// A middle-button drag always rotates the camera and must not invoke the
	// active modeling action, even when an item is selected.
	if (Fl::event_button2()) {
		camPos.x -= change.x;
		camPos.y += change.y;
		clamp(camPos.y, 1.0f, 179.0f);
		activeRenderer->SetCamera(sphericalToCartesian(camPos));

		firstDragCallback = false;
		mouseDragPrevPos = pos;
		mouseDragPrevPosUV = uv;
		redraw();
		return;
	}

	bool camera = true;

	// Transformation drag depending on action.
	if (activeAction == TRANSLATE_OUT) {
		camera = !translateOutDrag(pos, uv);
	}
	else if (activeAction == TRANSLATE_FREE) {
		camera = !translateFreeDrag(pos, uv);
	}
	else if (activeAction == TRANSLATE_EDGE) {
		camera = !translateAlongEdgeDrag(pos);
	}
	else if (activeAction == SCALE) {
		camera = !uniformScaleDrag(pos);
	}
	else if (activeAction == ROTATE) {
		camera = !rotateDrag(pos);
	}
	else if (activeAction == EXTRUDE) {
		camera = !extrudeDrag(pos, uv);
	}

	// Comment out to disable LMB drag camera rotation.
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
	updateArrows();
	redraw();
}

// AI-generated code: Positions the translation gizmo at the selection average.
void Viewport::updateArrows() {
	const bool translationAction =
		activeAction == TRANSLATE_AXIS;
	const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

	if (!translationAction || selectedItems.empty()) {
		modelingRenderer.ClearArrows();
		return;
	}

	Vec3f positionSum(0.0f);
	uint positionCount = 0;

	for (const SelectionItem& item : selectedItems) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];
		Vec3f worldPosition = mesh->Pos();

		switch (item.type) {
		case OBJECT:
			// An object is positioned at its local origin.
			break;
		case VERTEX:
			worldPosition += mesh->getTransform3x3() * mesh->GetVertexPosition(item.componentId);
			break;
		case EDGE:
			worldPosition += mesh->getTransform3x3() * mesh->GetEdgeCenter(item.componentId);
			break;
		case FACE:
			worldPosition += mesh->getTransform3x3() * mesh->faceCentroid(item.componentId);
			break;
		}

		positionSum += worldPosition;
		++positionCount;
	}

	if (positionCount == 0) {
		modelingRenderer.ClearArrows();
		return;
	}

	modelingRenderer.UpdateArrows(positionSum / float(positionCount), hoveredArrowAxis);
}

bool Viewport::uniformScaleDrag(const Vec2f& pos) {
	// If the action action is not scale, and there are no selections, do nothing.
	// Todo: edge and face scale.
	if (getActiveElement() != OBJECT || activeAction != SCALE || selectionManager.getSelectedItems().empty()) {
		return false;
	}

	// Calculate the amount to scale by.
	Vec2f change = pos - mouseDragPrevPos;
	float factor = 0.01f * change.Dot(initialDragDir);

	// Change all selected elements.
	// Use unordered set for fast lookup time.
	std::unordered_set<uint> transformedMeshes;
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		// Don't transform the same mesh multiple times.
		if (item.meshId >= scene.MeshCount() || !transformedMeshes.insert(item.meshId).second) {
			continue;
		}

		// Scale.
		scene[item.meshId]->AddScale(factor);
	}

	if (transformedMeshes.empty()) {
		return false;
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	updateArrows();

	if (primarySelectionCallback) { primarySelectionCallback(); }

	return true;
}

bool Viewport::translateOutDrag(const Vec2f& pos, const Vec2f& uv) {
	// If there are no selected elements, do nothing.
	if (selectionManager.getSelectedItems().empty()) { return false; }
	
	// If we are selecting objects, translate free instead of out.
	if (getActiveElement() == OBJECT) {
		return translateFreeDrag(pos, uv);
	}

	// Calculate change in position.
	Vec2f change = pos - mouseDragPrevPos;
	float scale = 0.01f * change.Dot(initialDragDir);

	// If we are selecting vertices, do vertex translation.
	if (getActiveElement() == VERTEX) {
		return translateOutVertexDrag(scale);
	}

	// else get all selected items.
	const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();
	// If this is the first drag, establish direction.
	if (firstDragCallback) {
		dragActionDirs.assign(selectedItems.size(), Vec3f(0.0f));
		// For each selected item, transform.
		for (size_t i = 0; i < selectedItems.size(); ++i) {
			const SelectionItem& item = selectedItems[i];
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			if (item.type == FACE) {
				dragActionDirs[i] = scene[item.meshId]->GetFaceNormal(item.componentId);
			}
			else if (item.type == EDGE) {
				dragActionDirs[i] = scene[item.meshId]->GetEdgeNormal(item.componentId);
			}
		}
	}

	bool transformed = false;
	// Otherwise, translate.
	for (size_t i = 0; i < selectedItems.size(); ++i) {
		const SelectionItem& item = selectedItems[i];
		if (item.meshId >= scene.MeshCount() || i >= dragActionDirs.size()) {
			continue;
		}

		if (item.type == FACE) {
			scene[item.meshId]->TranslateFace(item.componentId, dragActionDirs[i] * scale);
			transformed = true;
		}
		else if (item.type == EDGE) {
			scene[item.meshId]->TranslateEdge(item.componentId, dragActionDirs[i] * scale);
			transformed = true;
		}
	}

	if (!transformed) {
		return false;
	}

	if (primarySelectionCallback) { primarySelectionCallback(); }
	modelingRenderer.UpdateRenderBuffers(scene);

	return true;
}

bool Viewport::translateOutVertexDrag(float scale) {
	// If no selection, do nothing.
	const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();
	if (selectedItems.empty()) {
		return false;
	}

	// If first drag, establish direction, translate out.
	if (firstDragCallback) {
		dragActionDirs.assign(selectedItems.size(), Vec3f(0.0f));
		for (size_t i = 0; i < selectedItems.size(); ++i) {
			const SelectionItem& item = selectedItems[i];
			if (item.type == VERTEX && item.meshId < scene.MeshCount()) {
				dragActionDirs[i] = scene[item.meshId]->GetVertexNormal(item.componentId);
			}
		}
	}

	// Else translate out.
	bool transformed = false;
	for (size_t i = 0; i < selectedItems.size(); ++i) {
		const SelectionItem& item = selectedItems[i];
		if (item.type != VERTEX || item.meshId >= scene.MeshCount() || i >= dragActionDirs.size()) {
			continue;
		}

		scene[item.meshId]->TranslateVertex(item.componentId, scale * dragActionDirs[i]);
		transformed = true;
	}

	if (!transformed) {
		return false;
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	updateArrows();

	if (primarySelectionCallback) { primarySelectionCallback(); }

	return true;
}

bool Viewport::translateAlongEdgeDrag(const Vec2f& pos) {
	// If the active element is not vertex, do nothing.
	if (getActiveElement() != VERTEX)
		return false;
	
	Vec3f f, u, r; 
	modelingRenderer.GetCameraDirs(f, u, r);

	// Use unordered set for fast lookup.
	bool transformed = false;
	std::unordered_set<uint> transformedObjects;
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		// Get the selected mesh.
		HalfEdgeMesh* mesh = scene[item.meshId];

		// Calculate the direction.
		if (firstDragCallback) {
			Vec2f dragDir = Vec2f(initialDragDir.x, -initialDragDir.y);
			actionEdge = mesh->PickOutgoingHalfEdge(item.componentId, dragDir, r, u);
		}

		if (actionEdge == MAX_UINT)
			return false;

		// Translate the vertex along the edge.
		Vec2f change = pos - mouseDragPrevPos;
		float distance = 0.01f * change.Dot(initialDragDir);
		mesh->TranslateVertexAlongEdge(item.componentId, actionEdge, distance);
		transformed = true;
	}

	if (!transformed) {
		return false;
	}

	modelingRenderer.UpdateRenderBuffers(scene);
	
	if (primarySelectionCallback)
		primarySelectionCallback(); 
	
	return true; 
}

bool Viewport::translateFreeDrag(const Vec2f& pos, const Vec2f& uv) {
	Vec3f f, u, r;
	modelingRenderer.GetCameraDirs(f, u, r);

	// Calculate position change.
	Vec2f change;
	change.x = mouseDragPrevPos.x - pos.x;
	change.y = pos.y - mouseDragPrevPos.y;
	Vec3f translation = 0.01f * (change.x * r + change.y * u);

	// Use unordered set for fast lookup.
	bool transformed = false;
	std::unordered_set<uint> transformedObjects;
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		// Translate depending on which element type.
		HalfEdgeMesh* mesh = scene[item.meshId];
		switch (item.type) {
		case OBJECT:
			if (transformedObjects.insert(item.meshId).second) {
				mesh->TranslatePos(translation);
				transformed = true;
			}
			break;
		case VERTEX:
			mesh->TranslateVertex(item.componentId, mesh->WorldDirToLocalDir(translation));
			transformed = true;
			break;
		case EDGE:
			mesh->TranslateEdge(item.componentId, mesh->WorldDirToLocalDir(translation));
			transformed = true;
			break;
		case FACE:
			mesh->TranslateFace(item.componentId, mesh->WorldDirToLocalDir(translation));
			transformed = true;
			break;
		}
	}

	if (!transformed) {
		return false;
	}

	// Breaking stuff?
	/*switch (activeElement) {
	case OBJECT:
		scene[selectionIds.x - 1]->TranslatePos(translation);
		
		break;
	case EDGE:
		translation = scene[selectionIds.x - 1]->WorldDirToLocalDir(translation);
		scene[selectionIds.x - 1]->TranslateEdge(selectionIds.z, translation);
		break;
	case FACE:
		translation = scene[selectionIds.x - 1]->WorldDirToLocalDir(translation);
		scene[selectionIds.x - 1]->TranslateFace(selectionIds.y, translation);
		break;
	}*/

	modelingRenderer.UpdateRenderBuffers(scene);
	updateArrows();

	if (primarySelectionCallback) { primarySelectionCallback(); }

	return true;
}

bool Viewport::rotateDrag(const Vec2f& pos) {
	// If no selections, do nothing.
	if (selectionManager.getSelectedItems().empty()) {
		return false;
	}

	Vec3f f, u, r;
	modelingRenderer.GetCameraDirs(f, u, r);

	// calculate change in rotation.
	Vec2f change;
	change.y = mouseDragPrevPos.x - pos.x;
	change.x = -(pos.y - mouseDragPrevPos.y);

	Vec3f dir = change.x * r + change.y * u;
	float length = dir.Length();
	if (length == 0.0f) {
		return false;
	}
	dir /= length;

	// Use unordered set for fast lookup.
	bool transformed = false;
	std::unordered_set<uint> transformedObjects;
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		// Don't rotate twice.
		if (item.type != OBJECT || item.meshId >= scene.MeshCount() ||
			!transformedObjects.insert(item.meshId).second) {
			continue;
		}

		Vec3f eulerRot = scene[item.meshId]->Rot();
		Matrix3f origRot;
		origRot.SetRotationZYX(degreesToRadians(eulerRot.x), degreesToRadians(eulerRot.y), degreesToRadians(eulerRot.z));

		//Vec3f eulerRot = scene[item.meshId]->eulerRot;
		//Matrix3f origRot;
		//origRot.SetRotationZYX(degreesToRadians(eulerRot.x), degreesToRadians(eulerRot.y), degreesToRadians(eulerRot.z));

		Matrix3f rot;
		rot.SetRotation(dir, length * 0.01f);
		rot *= origRot;

		scene[item.meshId]->Rot(matrixToEulerAngle(rot));

		transformed = true;
	}

	if (!transformed) {
		return false;
	}

	modelingRenderer.UpdateRenderBuffers(scene);

	if (primarySelectionCallback) { primarySelectionCallback(); }

	return true;
}

bool Viewport::arrowDrag(Vec2u ids) {
	return false;
}

int Viewport::mousePush() {
	// Middle mouse starts a camera drag without changing the selection.
	if (Fl::event_button() == FL_MIDDLE_MOUSE) {
		mouseDragStartPos.Set(Fl::event_x(), Fl::event_y());
		mouseDragPrevPos = mouseDragStartPos;
		firstDragCallback = false;
		return 1;
	}

	// For now, we ignore other non-left mouse button events.
	if (Fl::event_button() != FL_LEFT_MOUSE) return 0;

	// Get position of mouse press.
	uint x = (float(Fl::event_x()) / w()) * pixel_w();
	uint y = (1.0f - (float(Fl::event_y()) / h())) * pixel_h();

	// Get the selection ids at this position.
	Vec3u selectionIds = modelingRenderer.SelectionIds(x, y);

	// AI-generated code: Keeps arrow clicks separate from mesh selection.
	// If the selection is an arrow.
	if (selectionIds.x == ARROW_IDENTIFIER) {
		// Handle Arrow Selection.
		std::cout << "Arrow: Transformation: " << selectionIds.y << ", Axis: " << selectionIds.z << std::endl;
	}
	// Else process like any other selection.
	else {
		// Create a selection item to send to the selection manager.
		std::optional<SelectionItem> item = createSelectionItem(selectionIds);

		// query the selection maanger with the item.
		selectionManager.processSelection(item, additiveSelection);

		// Tell the renderer to update the selection visuals.
		modelingRenderer.UpdateSelection(selectionManager.getSelectedItems());
		updateArrows();

		if (primarySelectionCallback) {
			primarySelectionCallback();
		}

		redraw();

		// Print selection ids.
		std::cout << "Object Id: " << selectionIds.x << std::endl;
		std::cout << "Component Id: " << selectionIds.y << std::endl;
		std::cout << "Sub-Component Id: " << selectionIds.z << std::endl;
	}

	// Drag stuff, refactor later.
	mouseDragStartPos.Set(Fl::event_x(), Fl::event_y());
	mouseDragPrevPos = mouseDragStartPos;
	mouseDragStartPosUV.Set(mouseDragStartPos.x / (w() - 1), mouseDragStartPos.y / (h() - 1));
	mouseDragStartPosUV.y = 1.0f - mouseDragStartPosUV.y;
	mouseDragPrevPosUV = mouseDragStartPosUV;
	firstDragCallback = true;
	//redraw();
	return 1;
}

void Viewport::mouseWheel() {
	// Adjust the camera zoom.
	camPos.z += Fl::event_dy();
	if (camPos.z < 0.0f) { camPos.z = 0.0f; }

	camPos.x += Fl::event_dx();

	activeRenderer->SetCamera(sphericalToCartesian(camPos));
	redraw();
}

void Viewport::keyDown() {
	switch (Fl::event_key()) {
	case 'e': 
		selectionManager.setActiveElement(VERTEX); 
		activeAction = TRANSLATE_EDGE; 
		break; 
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
		deleteElement();
		break;
	case 'd': 
		duplicateElement(); 
		break;
	case 'v':
		selectionManager.setActiveElement(EDGE);
		activeAction = TRANSLATE_OUT;
		break;
	case FL_Control_L:
		additiveSelection = true;
		break;
	default:
		return;
	}
	redraw();
}

void Viewport::keyUp() {
	switch (Fl::event_key()) {
	case FL_Control_L:
		additiveSelection = false;
		break;
	default:
		return;
	}
	redraw();
}

std::optional<SelectionItem> Viewport::createSelectionItem(Vec3u ids)
{
	switch (getActiveElement()) {
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
	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);

	//scene[0]->Extrude(5);
	//scene[0]->TranslateFace(5, scene[0]->GetFaceNormal(5));
	//HalfEdgeMeshBeveler beveler(*scene[0]);
	//beveler.StartEdgeBevel(17);
	//beveler.AdjustBevel(0.5f);
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
			mesh->Scale(component, scale);
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
	updateArrows();
	redraw();
}

void Viewport::bevel() {
	// todo
}

bool Viewport::extrudeDrag(const Vec2f& pos, const Vec2f& uv) {
	// Extrusion only works on selected faces.
	if (selectionManager.getSelectedItems().empty() || getActiveElement() != FACE) { return false; }

	// Project this mouse movement onto the initial drag direction.
	Vec2f change = pos - mouseDragPrevPos;
	float scale = 0.01f * change.Dot(initialDragDir);

	const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

	// Capture the face normals before changing. Extrude() replaces
	// the selected face's edge loop with the new cap, so the same face id can be
	// translated immediately after the extrusion.
	if (firstDragCallback) {
		dragActionDirs.assign(selectedItems.size(), Vec3f(0.0f));
		for (size_t i = 0; i < selectedItems.size(); ++i) {
			const SelectionItem& item = selectedItems[i];
			if (item.type != FACE || item.meshId >= scene.MeshCount()) {
				continue;
			}

			dragActionDirs[i] = scene[item.meshId]->GetFaceNormal(item.componentId);
		}
	}

	bool transformed = false;
	for (size_t i = 0; i < selectedItems.size(); ++i) {
		const SelectionItem& item = selectedItems[i];
		if (item.type != FACE || item.meshId >= scene.MeshCount() ||
			i >= dragActionDirs.size()) {
			continue;
		}

		HalfEdgeMesh* mesh = scene[item.meshId];
		if (firstDragCallback) {
			mesh->Extrude(item.componentId);
		}

		// Extrude() creates the bridge and cap at the original position. Move
		// that cap to make the extrusion visible and interactive.
		mesh->TranslateFace(item.componentId, dragActionDirs[i] * scale);
		transformed = true;
	}

	if (!transformed) {
		return false;
	}

	if (primarySelectionCallback) { primarySelectionCallback(); }
	modelingRenderer.UpdateRenderBuffers(scene);

	return true;
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
	updateArrows();
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
	updateArrows();
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
			mesh->Rot(component, angle);
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
	updateArrows();
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
			mesh->Pos(component, pos);
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
	updateArrows();
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
	selectionManager.setActiveElement(element);
	modelingRenderer.UpdateSelection(selectionManager.getSelectedItems());
	modelingRenderer.UpdateHover(hoveredSelectionIds, element);
	updateArrows();
	if (primarySelectionCallback) {
		primarySelectionCallback();
	}
	redraw();
}

void Viewport::setActiveAction(Action action) {
	activeAction = action;
	if (activeAction != TRANSLATE_AXIS) {
		hoveredArrowAxis = MAX_UINT;
	}
	updateArrows();
	redraw();
}

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
