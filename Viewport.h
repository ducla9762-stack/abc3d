#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QPushButton>

#include <algorithm>

#include "render.h"
#include "helper.h"
#include "mesh.h"
#include "selectionManager.h"
#include "shapes.h"
#include "sceneFile.h"

inline bool initGlew = true;

class Viewport : public QOpenGLWidget
{
	Q_OBJECT

signals:
	void primarySelectionPosChanged(const Vec3f& pos);
	void primarySelectionScaleChanged(const Vec3f& scale);
	void primarySelectionRotChanged(const Vec3f& rot);
	void primarySelectionChanged(bool primarySelection);
	void enableNonVertexSpinners(bool enable);

public:

	enum Action {
		CAMERA, SELECT, TRANSLATE_OUT, TRANSLATE_FREE, TRANSLATE_AXIS, TRANSLATE_EDGE,
		SCALE, EXTRUDE, BEVEL, SHAPE, ROTATE
	};

private:

	struct MouseAction {
		Vec2f mouseDragStartPos = Vec2f(0.0f);
		Vec2f mouseDragStartPosUV = Vec2f(0.0f);

		Vec2f mouseDragPrevPos = Vec2f(0.0f);
		Vec2f mouseDragPrevPosUV = Vec2f(0.0f);

		Vec2f mouseDragPos = Vec2f(0.0f);
		Vec2f mouseDragPosUV = Vec2f(0.0f);

		Vec2f mouseDragChange = Vec2f(0.0f);

		Vec2f initialDragDir = Vec2f(0.0f);
		Vec2f initialDragDirUV = Vec2f(0.0f);

		bool firstMoveEvent = false;
		// Keep track of whether the mouse was dragged.
		bool dragged = false;

		static Vec2f ToUV(const Vec2f& coords, int w, int h) {
			Vec2f uv;
			uv.x = coords.x / float(w - 1);
			uv.y = coords.y / float(h - 1);
			return uv;
		}

		void MousePress(float x, float y, int w, int h) {
			mouseDragStartPos.Set(x, y);
			mouseDragPrevPos = mouseDragStartPos;

			mouseDragStartPosUV = ToUV(mouseDragStartPos, w, h);
			mouseDragPrevPosUV = mouseDragStartPosUV;

			firstMoveEvent = true;
			// If just a mouse press, no drag.
			dragged = false;
		}

		void StartMouseDragStep(float x, float y, int w, int h) {
			mouseDragPos.Set(x, y);
			mouseDragPosUV = ToUV(mouseDragPos, w, h);
			mouseDragChange = mouseDragPos - mouseDragPrevPos;

			// Small threshold for mouse jitter and such.
			const float DRAG_THRESHOLD = 3.0f;
			// Check if the mouse was actually dragged.
			if (!dragged && (mouseDragPos - mouseDragStartPos).LengthSquared() >=
				DRAG_THRESHOLD * DRAG_THRESHOLD) {
				dragged = true;
			}

			if (firstMoveEvent) {
				initialDragDir = mouseDragPos - mouseDragStartPos;
				initialDragDir.Normalize();

				initialDragDirUV = mouseDragPosUV - mouseDragStartPosUV;
				initialDragDirUV.Normalize();
			}
		}

		void EndMouseDragStep() {
			mouseDragPrevPos = mouseDragPos;
			mouseDragPrevPosUV = mouseDragPosUV;
			firstMoveEvent = false;
		}
	};

	struct ActionParams {
		Action action = SELECT;
		std::vector<Vec3f> actionDirs;
		void ClearActionDirs() { std::vector<Vec3f>().swap(actionDirs); }
	};

	QPushButton* elementButtons[4];
	SelectionManager selectionManager;
	ModelingRenderer renderer;
	Scene scene;
	HalfEdgeMeshBeveler* beveler;
	Vec3f camPos = Vec3f(0.0f, 90.0f, 5.0f); // Spherical Coordinates
	MouseAction mouseAction;
	ActionParams actionParams;
	bool verticesAlwaysVisible = false;

	// AI-generated code: Cursor hover state for arrows and active elements.
	uint hoveredArrowAxis = MAX_UINT;
	Vec3u hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	// AI-generated code: The arrow pressed at the start of an axis drag must
	// remain active even if the cursor moves away from the gizmo.
	uint draggedArrowAxis = MAX_UINT;
	bool draggingArrow = false;

	/// <summary>
	/// Update the selection visuals for the renderer.
	/// </summary>
	void updateSelectionVisuals() {
		// Get the selected items from the selection manager, these determine what gets operated on.
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		// Allocate a buffer for items that get rendered.
		// Edges also have a twin, which should appear selected, but will not be operated on.
		std::vector<SelectionItem> renderItems;
		renderItems.reserve(selectedItems.size() * 2);

		const Element activeElement = selectionManager.getActiveElement();

		// Check all selected items.
		for (const SelectionItem& item : selectedItems) {
			// If the selection size is exceeded, don't do anything.
			if (renderItems.size() >= MAX_SELECTION_ITEMS) {
				break;
			}

			// Else, put the selection in the buffer.
			renderItems.push_back(item);

			// If the element is not an edge, don't continue on looking for the twin.
			if (activeElement != EDGE || item.meshId >= scene.MeshCount()) {
				continue;
			}

			// Get the twin of the edge, if valid add to the buffer.
			const uint twinId = scene[item.meshId]->GetEdge(item.componentId).twin;
			if (twinId != MAX_UINT && twinId != item.componentId &&
				renderItems.size() < MAX_SELECTION_ITEMS) {
				renderItems.push_back(SelectionItem{ item.meshId, twinId });
			}
		}

		// Send the render buffer to the renderer for rendering (render render render).
		renderer.UpdateSelection(renderItems, activeElement);
	}

	// AI-generated: Reads the ID buffer at the Qt cursor position and forwards
	// the hovered object/component state to both mesh and vertex shaders.
	bool updateHoverVisual(const Vec2f& pos) {
		const float pixelRatio = static_cast<float>(devicePixelRatio());
		const int pixelWidth = std::max(1, static_cast<int>(width() * pixelRatio));
		const int pixelHeight = std::max(1, static_cast<int>(height() * pixelRatio));
		const int rawX = static_cast<int>(pos.x * pixelRatio);
		const int rawY = static_cast<int>(pos.y * pixelRatio);
		const uint x = static_cast<uint>(std::clamp(rawX, 0, pixelWidth - 1));
		const uint y = static_cast<uint>(std::clamp(rawY, 0, pixelHeight - 1));
		const Vec3u ids = renderer.SelectionIds(x, y);
		const uint newHoveredAxis =
			(actionParams.action == TRANSLATE_AXIS &&
			 ids.x == ARROW_IDENTIFIER && ids.y == 0 && ids.z < 3)
			? ids.z : MAX_UINT;

		const bool idsChanged = ids.x != hoveredSelectionIds.x ||
			ids.y != hoveredSelectionIds.y || ids.z != hoveredSelectionIds.z;
		const bool arrowChanged = newHoveredAxis != hoveredArrowAxis;
		hoveredSelectionIds = ids;
		hoveredArrowAxis = newHoveredAxis;

		const bool rendererChanged = renderer.UpdateHover(
			ids, selectionManager.getActiveElement(), scene);
		if (arrowChanged) {
			updateArrows();
		}
		return idsChanged || arrowChanged || rendererChanged;
	}

	void initializeGL() override {
		if (initGlew) {
			GLenum error = glewInit();
			if (error != GLEW_OK) {
				std::cout << "Failed to initialize glew: " << glewGetErrorString(error) << std::endl;
				return;
			}
			initGlew = false;
		}

		const GLubyte* version = glGetString(GL_VERSION);
		std::cout << "OpenGL Version: " << version << std::endl;

		makeCurrent();
		bool success = renderer.Initialize(defaultFramebufferObject());
		if (!success) {
			std::cout << "Failed to initialize renderer" << std::endl;
			return;
		}

		renderer.SetVertexPointsVisible(verticesAlwaysVisible);
		renderer.SetCamera(sphericalToCartesian(camPos));
		renderer.UpdateRenderBuffers(scene);
		updateSelectionVisuals();
		renderer.UpdateHover(Vec3u(0u), selectionManager.getActiveElement(), scene);
		updateArrows();
	}

	// AI-generated code: Positions the translation gizmo at the selection average.
	void updateArrows() {
		const bool translationAction =
			actionParams.action == TRANSLATE_AXIS;
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		if (!translationAction || selectedItems.empty()) {
			renderer.ClearArrows();
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

			switch (selectionManager.getActiveElement()) {
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
			renderer.ClearArrows();
			return;
		}

		const uint highlightedAxis = draggingArrow ? draggedArrowAxis : hoveredArrowAxis;
		renderer.UpdateArrows(positionSum / float(positionCount), highlightedAxis);
	}

	void paintGL() override {
		renderer.Draw(defaultFramebufferObject());
	}

	void resizeGL(int w, int h) override {
		renderer.Resize(devicePixelRatio() * w, devicePixelRatio() * h, defaultFramebufferObject());
	}

	SelectionItem createSelectionItem(const Vec3u& ids);

	void emitPrimarySelectionPosChanged() {
		if (selectionManager.meshCount() != 1) { return; }

		if (selectionManager.getActiveElement() == VERTEX) {
			const SelectionItem& item = selectionManager.getSelectedItems()[0];
			Vec3f pos = scene[item.meshId]->GetVertexPosition(item.componentId);
			pos = scene[item.meshId]->LocalPosToWorldPos(pos);
			emit primarySelectionPosChanged(pos);
		}
		else if (selectionManager.getActiveElement() == OBJECT) {
			const SelectionItem& item = selectionManager.getSelectedItems()[0];
			Vec3f pos = scene[item.meshId]->Pos();
			emit primarySelectionPosChanged(pos);
		}
	}

	void emitPrimarySelectionScaleChanged() {
		if (selectionManager.meshCount() != 1 || selectionManager.getActiveElement() != OBJECT) { return; }

		const SelectionItem& item = selectionManager.getSelectedItems()[0];
		Vec3f scale = scene[item.meshId]->Scale();
		emit primarySelectionScaleChanged(scale);
	}

	void emitPrimarySelectionRotChanged() {
		if (selectionManager.meshCount() != 1 || selectionManager.getActiveElement() != OBJECT) { return; }

		const SelectionItem& item = selectionManager.getSelectedItems()[0];
		Vec3f rot = scene[item.meshId]->Rot();
		emit primarySelectionRotChanged(rot);
	}

	void emitSelectionChanged() {
		bool primSelect = selectionManager.meshCount() == 1;
		bool isNotVertex = selectionManager.getActiveElement() != VERTEX;

		if (isNotVertex) {
			emit primarySelectionChanged(primSelect);
		}
		else {
			emit primarySelectionChanged(primSelect && selectionManager.itemCount() == 1);
		}

		emit enableNonVertexSpinners(primSelect && isNotVertex);

		emitPrimarySelectionPosChanged();
		emitPrimarySelectionScaleChanged();
		emitPrimarySelectionRotChanged();
	}

	void setActiveElement(Element element) {
		selectionManager.setActiveElement(element);

		if (!verticesAlwaysVisible) {
			renderer.SetVertexPointsVisible(element == VERTEX);
		}

		// Changing element mode clears the selection in SelectionManager. Push the
		// empty selection to the renderer immediately so the old highlight vanishes.
		makeCurrent();
		updateSelectionVisuals();
		renderer.UpdateHover(Vec3u(0u), element, scene);
		hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
		hoveredArrowAxis = MAX_UINT;
		draggingArrow = false;
		draggedArrowAxis = MAX_UINT;
		updateArrows();
		update();

		emit primarySelectionChanged(false);
	}

	// Brett's selection logic ported from FLTK Version's Viewport::mousePush
	void handleSelection(const Vec3u& selectionIds) {
		// Create a selection item to send to the selection manager.
		SelectionItem item = createSelectionItem(selectionIds);
		
		if (item.meshId == MAX_UINT) {
			// Clicking the background is a deselection. Keep the renderer and
			// gizmo in sync when the last selected item is removed.
			if (!selectionManager.getSelectedItems().empty()) {
				selectionManager.clearSelection();
				updateSelectionVisuals();
				updateArrows();
				emitSelectionChanged();
			}
			return;
		}

		// Store one half-edge for each logical edge.
		// Render both half-edges as selected.
		if (selectionManager.getActiveElement() == EDGE && item.meshId < scene.MeshCount()) {
			const uint twinId = scene[item.meshId]->GetEdge(item.componentId).twin;
			if (twinId != MAX_UINT && twinId < item.componentId) {
				item.componentId = twinId;
			}
		}

		selectionManager.processSelection(item);

		// Tell the renderer to update the selection visuals.
		updateSelectionVisuals();
		updateArrows();

		emitSelectionChanged();
	}

	void mousePressEvent(QMouseEvent* event) override {
		// Only accept event if the left mouse button is clicked
		if (!(event->button() & Qt::LeftButton)) {
			event->ignore();
			return;
		}

		// Accept the event and make the OpenGL context current in case the renderer needs it for selection behavior
		event->accept();
		makeCurrent();

		// Retrieve the current logical (not physical) pixel position
		Vec2f pos(event->position().x(), this->height() - 1.0f - event->position().y());

		// Update MouseAction struct
		mouseAction.MousePress(pos.x, pos.y, this->width(), this->height());

		// Capture an axis on press. The gizmo moves while it is being dragged,
		// so looking up the ID on release would lose the original constraint.
		const float pixelRatio = static_cast<float>(devicePixelRatio());
		const int pixelWidth = std::max(1, static_cast<int>(width() * pixelRatio));
		const int pixelHeight = std::max(1, static_cast<int>(height() * pixelRatio));
		const int rawX = static_cast<int>(pos.x * pixelRatio);
		const int rawY = static_cast<int>(pos.y * pixelRatio);
		const uint x = static_cast<uint>(std::clamp(rawX, 0, pixelWidth - 1));
		const uint y = static_cast<uint>(std::clamp(rawY, 0, pixelHeight - 1));
		const Vec3u ids = renderer.SelectionIds(x, y);
		const bool arrowHit =
			actionParams.action == TRANSLATE_AXIS &&
			ids.x == ARROW_IDENTIFIER && ids.y == 0 && ids.z < 3;
		draggingArrow = arrowHit;
		draggedArrowAxis = arrowHit ? ids.z : MAX_UINT;

		// Update Viewport.
		update();
	}

	void mouseReleaseEvent(QMouseEvent* event) override {
		// Only accept event if the left mouse button is clicked
		if (!(event->button() & Qt::LeftButton)) {
			event->ignore();
			return;
		}

		// Accept the event and make the OpenGL context current in case the renderer needs it for selection behavior
		event->accept();
		makeCurrent();

		// Retrieve the current logical (not physical) pixel position
		Vec2f pos(event->position().x(), this->height() - 1.0f - event->position().y());

		// Retrieve selection ids from corresponding physical pixel and print them
		Vec3u ids = renderer.SelectionIds(devicePixelRatio() * pos.x, devicePixelRatio() * (pos.y));

		// An axis click is a gizmo interaction, not a mesh selection. The axis
		// was captured on press and must remain captured even if the cursor moved.
		if (!draggingArrow) {
			// Only process selection if the mouse was clicked, not dragged.
			// Also make sure the selection is valid (not background).
			if (!mouseAction.dragged && ids.x != 0) {
				handleSelection(ids);
			}

			// Print selection ids.
			std::cout << "Object Id: " << ids.x << std::endl;
			std::cout << "Component Id: " << ids.y << std::endl;
			std::cout << "Sub-Component Id: " << ids.z << std::endl;
		}

		draggingArrow = false;
		draggedArrowAxis = MAX_UINT;

		// Update Viewport
		update();
	}

	bool translateFreeDrag() {
		if (selectionManager.getSelectedItems().empty()) { return false; }

		Vec3f f, u, r;
		renderer.GetCameraDirs(f, u, r);

		const float SCALE = 0.01f;
		const Vec2f& uv = mouseAction.mouseDragChange;
		Vec3f translation = -SCALE * (uv.x * r + uv.y * u);

		bool transformed = false;

		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			// Translate depending on which element type.
			HalfEdgeMesh* mesh = scene[item.meshId];
			switch (selectionManager.getActiveElement()) {
			case OBJECT:
				mesh->TranslatePos(translation);
				break;
			case VERTEX:
				mesh->TranslateVertex(item.componentId, mesh->WorldDirToLocalDir(translation));
				break;
			case EDGE:
				mesh->TranslateEdge(item.componentId, mesh->WorldDirToLocalDir(translation));
				break;
			case FACE:
				mesh->TranslateFace(item.componentId, mesh->WorldDirToLocalDir(translation));
				break;
			}

			transformed = true;
		}

		return transformed;
	}

	void initActionDirsToSelectedNormals() {
		// Get all selected items.
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		bool noValidItems = true;
		actionParams.actionDirs.assign(selectedItems.size(), Vec3f(0.0f));

		// For each selected item, get normal direction
		for (size_t i = 0; i < selectedItems.size(); ++i) {
			const SelectionItem& item = selectedItems[i];
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			switch (selectionManager.getActiveElement()) {
			case VERTEX:
				actionParams.actionDirs[i] = scene[item.meshId]->GetVertexNormal(item.componentId);
				break;
			case EDGE:
				actionParams.actionDirs[i] = scene[item.meshId]->GetEdgeNormal(item.componentId);
				break;
			case FACE:
				actionParams.actionDirs[i] = scene[item.meshId]->GetFaceNormal(item.componentId);
				break;
			}

			noValidItems = false;
		}

		if (noValidItems) { actionParams.ClearActionDirs(); }
	}

	void translateSelections() {
		// Compute translation's scale factor
		const float SCALE_COEF = 0.01f;
		float scale = SCALE_COEF * mouseAction.mouseDragChange.Dot(mouseAction.initialDragDir);

		// Translate selected elements
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();
		for (size_t i = 0; i < actionParams.actionDirs.size(); ++i) {
			const SelectionItem& item = selectedItems[i];
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			switch (selectionManager.getActiveElement()) {
			case VERTEX:
				scene[item.meshId]->TranslateVertex(item.componentId, actionParams.actionDirs[i] * scale);
				break;
			case EDGE:
				scene[item.meshId]->TranslateEdge(item.componentId, actionParams.actionDirs[i] * scale);
				break;
			case FACE:
				scene[item.meshId]->TranslateFace(item.componentId, actionParams.actionDirs[i] * scale);
				break;
			}
		}
	}

	bool translateOutDrag() {
		// If there are no selected elements, do nothing.
		if (selectionManager.getSelectedItems().empty()) { return false; }

		// If we are selecting objects, translate free instead of out.
		if (selectionManager.getActiveElement() == OBJECT) {
			return translateFreeDrag();
		}

		// If this is the first drag step, compute normal directions
		if (mouseAction.firstMoveEvent) {
			initActionDirsToSelectedNormals();
		}

		if (actionParams.actionDirs.empty()) { return false; }

		// Get all selected items.
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		// Translate
		translateSelections();

		return true;
	}

	bool translateEdgeDrag() { return false; }

	void setupBevelDrag() {
		if (beveler) { delete beveler; }

		uint meshId = selectionManager.getSelectedItems()[0].meshId;
		if (meshId >= scene.MeshCount()) { return; }

		beveler = new HalfEdgeMeshBeveler(*scene[meshId]);

		Element element = selectionManager.getActiveElement();
		if (element == OBJECT) {
			beveler->StartObjectBevel();
			return;
		}

		std::vector<uint> elements;
		elements.reserve(selectionManager.getSelectedItems().size());

		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (meshId >= scene.MeshCount()) { continue; }
			elements.push_back(item.componentId);
		}

		switch (element) {
		case EDGE:
			beveler->StartEdgeBevel(elements);
			break;
		case FACE:
			beveler->StartFaceBevel(elements);
			break;
		case VERTEX:
			beveler->StartVertexBevel(elements);
			break;
		}

		return;
	}

	bool bevelDrag() {
		if (selectionManager.getSelectedItems().empty()) { return false; }

		if (mouseAction.firstMoveEvent) {
			setupBevelDrag();
		}

		if (!beveler) { return false; }

		float factor = (mouseAction.mouseDragPosUV - mouseAction.mouseDragStartPosUV).Length();
		factor = std::min(factor, 0.25f) / 0.25f;

		beveler->AdjustBevel(factor);

		return true;
	}

	void extrudeSelectedElements() {
		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) { continue; }

			scene[item.meshId]->Extrude(item.componentId);
		}
	}

	bool extrudeDrag() {
		if (selectionManager.getSelectedItems().empty()) { return false; }

		// Extruding temporarily applies only to faces
		if (selectionManager.getActiveElement() != FACE) { return false; }

		// Compute normals if this is the first move event
		if (mouseAction.firstMoveEvent) {
			initActionDirsToSelectedNormals();
		}

		if (actionParams.actionDirs.empty()) { return false; }

		// Extrude selected elements if this is the first move event
		if (mouseAction.firstMoveEvent) {
			extrudeSelectedElements();
		}

		// Translated extruded faces
		translateSelections();

		return true;
	}

	bool scaleDrag() {
		if (selectionManager.getActiveElement() != OBJECT || selectionManager.getSelectedItems().empty()) {
			return false;
		}

		float factor = 0.01f * mouseAction.mouseDragChange.Dot(mouseAction.initialDragDir);

		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId < scene.MeshCount()) {
				scene[item.meshId]->AddScale(factor);
			}
		}

		renderer.UpdateRenderBuffers(scene);

		return true;
	}

	bool rotateDrag() { 
		if (selectionManager.getActiveElement() != OBJECT || selectionManager.getSelectedItems().empty()) {
			return false;
		}const uint NO_HOVER = 4294967295u;


		Vec3f f, u, r;
		renderer.GetCameraDirs(f, u, r);

		Vec3f dir = mouseAction.mouseDragChange.y * r - mouseAction.mouseDragChange.x * u;
		float length = dir.Length();
		if (length == 0.0f) {
			return false;
		}
		dir /= length;

		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) { continue; }

			Vec3f eulerRot = scene[item.meshId]->Rot();
			Matrix3f origRot;
			origRot.SetRotationZYX(degreesToRadians(eulerRot.x), degreesToRadians(eulerRot.y), degreesToRadians(eulerRot.z));

			Matrix3f rot;
			rot.SetRotation(dir, length * 0.01f);
			rot *= origRot;

			scene[item.meshId]->Rot(matrixToEulerAngle(rot));
		}

		renderer.UpdateRenderBuffers(scene);

		return true;
	}

	// Constrain the drag to the world-space axis captured on mouse press.
	bool arrowDrag() {
		if (!draggingArrow || draggedArrowAxis >= 3 ||
			selectionManager.getSelectedItems().empty()) {
			return false;
		}

		Vec3f forward, up, right;
		renderer.GetCameraDirs(forward, up, right);

		Vec3f axis;
		switch (draggedArrowAxis) {
		case 0: axis = Vec3f(1.0f, 0.0f, 0.0f); break;
		case 1: axis = Vec3f(0.0f, 1.0f, 0.0f); break;
		case 2: axis = Vec3f(0.0f, 0.0f, 1.0f); break;
		default: return false;
		}

		// Project the selected world axis onto the screen to determine how much
		// the mouse moved along it. An axis aimed directly at the camera has no
		// stable screen direction.
		Vec2f axisScreen(axis.Dot(right), axis.Dot(up));
		const float axisScreenLength = axisScreen.Length();
		if (axisScreenLength < 1e-6f) {
			return false;
		}
		axisScreen /= axisScreenLength;

		Vec2f change;
		change.x = -mouseAction.mouseDragChange.x;
		// Qt coordinates were converted to bottom-left origin in the mouse
		// handlers, so invert the vertical delta to match the FLTK drag math.
		change.y = -mouseAction.mouseDragChange.y;
		const Vec3f worldTranslation = axis * (0.01f * change.Dot(axisScreen));

		bool transformed = false;
		std::unordered_set<uint> transformedObjects;
		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			HalfEdgeMesh* mesh = scene[item.meshId];
			switch (selectionManager.getActiveElement()) {
			case OBJECT:
				if (transformedObjects.insert(item.meshId).second) {
					mesh->TranslatePos(worldTranslation);
					transformed = true;
				}
				break;
			case VERTEX:
				mesh->TranslateVertex(item.componentId,
					mesh->WorldDirToLocalDir(worldTranslation));
				transformed = true;
				break;
			case EDGE:
				mesh->TranslateEdge(item.componentId,
					mesh->WorldDirToLocalDir(worldTranslation));
				transformed = true;
				break;
			case FACE:
				mesh->TranslateFace(item.componentId,
					mesh->WorldDirToLocalDir(worldTranslation));
				transformed = true;
				break;
			}
		}

		if (transformed) {
			emitPrimarySelectionPosChanged();
		}
		return transformed;
	}

	void mouseMoveEvent(QMouseEvent* event) override {
		// Accept the event and make the OpenGL context current for renderer.
		event->accept();
		makeCurrent();

		// AI-generated: Separates passive hover tracking from active drag handling
		// so enabling Qt mouse tracking does not turn ordinary movement into camera movement.
		const Vec2f pos(event->position().x(), this->height() - 1.0f - event->position().y());
		const bool dragging = event->buttons() & Qt::LeftButton;
		
		// Update hover visuals.
		const bool hoverChanged = updateHoverVisual(pos);

		// With mouse tracking enabled, mouseMoveEvent also runs without a button
		// pressed. Hovering must not be interpreted as a camera drag.
		if (!dragging) {
			if (hoverChanged) {
				update();
			}
			return;
		}

		// Update MouseAction struct
		mouseAction.StartMouseDragStep(pos.x, pos.y, this->width(), this->height());

		// Perform drag action
		bool actionPerformed = false;

		// Later we'll want to not update render buffers for simple transforms like object level scale, rotate, and translate.
		// We'll do this with a buffer containing objects' model transformations in the shader & renderer
		bool updateRenderBuffers = true;

		switch (actionParams.action) {
		case TRANSLATE_OUT:
			actionPerformed = translateOutDrag();
			if (actionPerformed) { emitPrimarySelectionPosChanged(); }
			break;
		case TRANSLATE_FREE:
			actionPerformed = translateFreeDrag();
			if (actionPerformed) { emitPrimarySelectionPosChanged(); }
			break;
		case TRANSLATE_EDGE:
			actionPerformed = translateEdgeDrag();
			if (actionPerformed) { emitPrimarySelectionPosChanged(); }
			break;
		case TRANSLATE_AXIS:
			actionPerformed = arrowDrag();
			break;
		case SCALE:
			actionPerformed = scaleDrag();
			if (actionPerformed) { emitPrimarySelectionScaleChanged(); }
			break;
		case ROTATE:
			actionPerformed = rotateDrag();
			if (actionPerformed) { emitPrimarySelectionRotChanged(); }
			break;
		case EXTRUDE:
			actionPerformed = extrudeDrag();
			break;
		case BEVEL:
			actionPerformed = bevelDrag();
			break;
		default:
			updateRenderBuffers = false;
			break;
		}

		// If no action performed, move the camera
		if (!actionPerformed) {
			camPos.x -= mouseAction.mouseDragChange.x;
			camPos.y += mouseAction.mouseDragChange.y;
			clamp(camPos.y, 1.0f, 179.0f);
			renderer.SetCamera(sphericalToCartesian(camPos));
		}
		else if (updateRenderBuffers) {
			renderer.UpdateRenderBuffers(scene);
		}

		updateArrows();

		// Update MouseAction struct
		mouseAction.EndMouseDragStep();

		update();
	}

	void leaveEvent(QEvent* event) override {
		event->accept();
		makeCurrent();
		const bool hoverChanged = renderer.UpdateHover(
			Vec3u(0u), selectionManager.getActiveElement(), scene);
		const bool arrowChanged = hoveredArrowAxis != MAX_UINT;
		hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
		hoveredArrowAxis = MAX_UINT;
		if (arrowChanged) {
			updateArrows();
		}
		if (hoverChanged || arrowChanged) {
			update();
		}
	}

	void wheelEvent(QWheelEvent* event) override {
		// Accept event and get change in wheel
		event->accept();
		QPoint change = event->angleDelta();

		// Convert change in wheel to change in camera
		Vec2f camChange(change.x(), change.y());
		const float SCALE = 0.025f;
		camChange *= SCALE;

		// Update camera
		camPos.z -= float(camChange.y);
		if (camPos.z < 0.0f) { camPos.z = 0.0f; }

		camPos.x += camChange.x;

		// Update renderer and viewport
		makeCurrent();
		renderer.SetCamera(sphericalToCartesian(camPos));

		update();
	}

	void resizeEvent(QResizeEvent* event) override {
		QOpenGLWidget::resizeEvent(event);

		const int X_PADDING = 5;
		const int Y_PADDING = 5;

		for (int i = 0; i < 4; ++i) {
			int x = Y_PADDING * (i + 1) + 64 * i;
			int y = this->height() - 1 - Y_PADDING - 64;
			elementButtons[i]->move(x, y);
		}
	}

	void mouseDoubleClickEvent(QMouseEvent* event) {
		selectionManager.clearSelection();
		makeCurrent();
		updateSelectionVisuals();
		updateArrows();
		update();

		emit primarySelectionChanged(false);
	}

	template<typename T, typename U>
	void loadMesh(const T& vertices, const U& faces) {
		scene.NewMesh()->load(vertices, faces);
		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
	}

	void setAction(Action action) {
		actionParams.action = action;
		hoveredArrowAxis = MAX_UINT;
		draggingArrow = false;
		draggedArrowAxis = MAX_UINT;
		if (context() != nullptr && context()->isValid()) {
			makeCurrent();
			updateArrows();
		}
		update();
	}

public:
	Viewport(QWidget* parent);
	~Viewport();

public slots:
	void setActionExtrude() { setAction(EXTRUDE); }
	void setActionBevel() { setAction(BEVEL); }
	void setActionTranslateFree() { setAction(TRANSLATE_FREE); }
	void setActionTranslateOut() { setAction(TRANSLATE_OUT); }
	void setActionTranslateEdge() { setAction(TRANSLATE_EDGE); }
	void setActionTranslateAxis() { setAction(TRANSLATE_AXIS); }
	void setActionScale() { setAction(SCALE); }
	void setActionRotate() { setAction(ROTATE); }
	void setActionSelect() { setAction(SELECT); }
	void setActionCamera() { setAction(CAMERA); }

	void setElementObject() { setActiveElement(OBJECT); }
	void setElementFace() { setActiveElement(FACE); }
	void setElementEdge() { setActiveElement(EDGE); }
	void setElementVertex() { setActiveElement(VERTEX); }

	void tessellateSelection() {
		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			switch (selectionManager.getActiveElement()) {
			case OBJECT:
				scene[item.meshId]->tesselate();
				break;

			case EDGE:
				scene[item.meshId]->refineEdge(item.componentId);
				break;

			case FACE:
				//scene[item.meshId]->tessFace(item.componentId);
				break;

			default:
				return;
			}
		}

		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
	}

	void subdivideSelection() {
		if (selectionManager.getActiveElement() != OBJECT) { return; }

		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId < scene.MeshCount()) {
				scene[item.meshId]->subdivide();
			}
		}

		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
	}

	void triangulateSelection() {
		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			switch (selectionManager.getActiveElement()) {
			case OBJECT:
				scene[item.meshId]->Triangulate();
				break;

			case FACE:
				scene[item.meshId]->TriangulateFace(item.componentId);
				break;

			default:
				return;
			}
		}

		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
	}

	void flipSelection() {
		if (selectionManager.getActiveElement() != EDGE || selectionManager.itemCount() == 0) { return; }

		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) { continue; }

			scene[item.meshId]->flipEdge(item.componentId);
		}

		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
	}

	void setPrimarySelectionPos(const Vec3f& pos) {
		if (selectionManager.meshCount() != 1) { return; }

		const SelectionItem& item = selectionManager.getSelectedItems()[0];

		if (selectionManager.getActiveElement() == VERTEX) {
			if (selectionManager.itemCount() != 1) { return; }
			Vec3f vertPos = scene[item.meshId]->WorldDirToLocalDir(pos);
			scene[item.meshId]->SetVertexPosition(item.componentId, vertPos);
		}
		else {
			scene[item.meshId]->Pos(pos);
		}

		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
	}

	void setPrimarySelectionScale(const Vec3f& scale) {
		if (selectionManager.meshCount() != 1) { return; }

		if (selectionManager.getActiveElement() != VERTEX) {
			const SelectionItem& item = selectionManager.getSelectedItems()[0];
			scene[item.meshId]->Scale(scale);
			makeCurrent();
			renderer.UpdateRenderBuffers(scene);
			update();
		}
	}

	void setPrimarySelectionRot(const Vec3f& rot) {
		if (selectionManager.meshCount() != 1) { return; }

		if (selectionManager.getActiveElement() != VERTEX) {
			const SelectionItem& item = selectionManager.getSelectedItems()[0];
			scene[item.meshId]->Rot(rot);
			makeCurrent();
			renderer.UpdateRenderBuffers(scene);
			update();
		}
	}

	void loadCube() { loadMesh(CUBE_VERTICES, CUBE_FACES); }
	void loadPyramid() { loadMesh(PYRAMID_VERTICES, PYRAMID_FACES); }
	void loadCylinder(){
		std::vector<Vec3f> vertices;
		std::vector<Vec4u> faces;
		generateUnitCylinder(vertices, faces, 12);
		loadMesh(vertices, faces);
	}
	void loadTorus(){
		std::vector<Vec3f> vertices;
		std::vector<Vec4u> faces;
		generateUnitTorus(vertices, faces, 40, 20);
		loadMesh(vertices, faces);
	}
	void loadUvSphere() { loadMesh(ICOSPHERE_VERTICES, ICOSPHERE_FACES); }
	void loadIcoSphere(){
		std::vector<Vec3f> vertices;
		std::vector<Vec4u> faces;
		generateUVSphere(vertices, faces, 10, 10);
		loadMesh(vertices, faces);
	}

	/// <summary>
	/// Loads the file at filepath as a mesh.
	/// </summary>
	/// <param name="filepath"></param>
	/// <returns></returns>
	ImportReport loadFile(const char* filepath) {
		// Build into local meshes first so a rejected file leaves the scene untouched.
		// Adding to the scene up front would strand empty meshes on every failed import.
		std::vector<HalfEdgeMesh> candidates;
		ImportReport report = loadMeshes(filepath, candidates);
		if (!report.ok()) { return report; }

		for (HalfEdgeMesh& candidate : candidates) {
			*scene.NewMesh() = std::move(candidate);
		}

		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
		return report;
	}

	/// <summary>
	/// Replaces the current scene with ABC3D scene file. 
	/// NOTE: Every mesh is built before anything is discarded, so a rejected/invalid file will STILL leave the current scene intact
	/// </summary>
	/// <param name="filepath">true if loaded</param>
	/// <returns></returns>
	bool openScene(const char* filepath) {
		SceneFile doc;
		if (!ReadSceneFile(filepath, doc)) {
			return false;
		}

		//Build every mesh before touching a scene. If invalid file, we don't load. 
		std::vector<HalfEdgeMesh> candidates;
		candidates.reserve(doc.objects.size());

		//Saved ids are sparse because deleted are never written. 
		//Remember where each saved id maps to so we can remap to the dense ids
		std::unordered_map<uint, uint> idMap;

		for (const SceneFileObject& obj : doc.objects) {
			std::istringstream geometry(obj.objText);
			FaceList faceList;

			if (!faceList.LoadObjStream(geometry)) {
				return false;
			}

			candidates.emplace_back();
			if (!candidates.back().load(faceList)) {
				return false;
			}

			candidates.back().Pos(obj.pos);
			candidates.back().Rot(obj.rot);
			candidates.back().Scale(obj.scale);

			idMap[obj.id] = static_cast<uint>(candidates.size() - 1);
		}

		scene.Clear();
		for (HalfEdgeMesh& candidate : candidates) {
			*scene.NewMesh() = std::move(candidate);
		}

		camPos = doc.camera;
		renderer.SetCamera(sphericalToCartesian(camPos));
		renderer.SetGridVisible(doc.grid);
		//renderer.SetVertexPointsVisible(doc.points);

		//setElementObject();
		selectionManager.clearSelection();

		for (uint savedId : doc.selection) {
			auto remapped = idMap.find(savedId);
			if (remapped != idMap.end()) {
				selectionManager.selectMesh(remapped->second);
			}
		}

		emitSelectionChanged();

		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		updateSelectionVisuals();
		updateArrows();
		update();

		return true;
	}

	void setCameraPosition(const Vec3f& camPos) {

	}

	void setAlwaysShowVertices(bool show) {
		verticesAlwaysVisible = show;
		if(selectionManager.getActiveElement() != VERTEX){
			renderer.SetVertexPointsVisible(verticesAlwaysVisible);
			update();
		}
	}

	// directly ported from FLTK Version's Viewport::deleteElement
	void deleteSelection() {
		std::cout << "delete" << std::endl;
		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			switch (selectionManager.getActiveElement()) {
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

		makeCurrent();
		selectionManager.clearSelection();
		renderer.UpdateRenderBuffers(scene);
		updateSelectionVisuals();
		updateArrows();
		update();
	}
};

