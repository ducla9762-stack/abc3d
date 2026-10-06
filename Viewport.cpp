#include "Viewport.h"
#include "shapes.h"
#include <qpushbutton.h>
#include <QMenu>

Viewport::Viewport(QWidget *parent)
	: QOpenGLWidget(parent)
{
	QSurfaceFormat format;
	format.setVersion(4, 6);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setRedBufferSize(8);
	format.setBlueBufferSize(8);
	format.setGreenBufferSize(8);
	format.setAlphaBufferSize(8);
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(0);
	format.setSamples(0);
	setFormat(format);

	// Element Buttons

	elementButtons[0] = new QPushButton("Vertex", this);
	elementButtons[1] = new QPushButton("Edge", this);
	elementButtons[2] = new QPushButton("Face", this);
	elementButtons[3] = new QPushButton("Object", this);

	QObject::connect(elementButtons[0], &QPushButton::released, this, &Viewport::setElementVertex);
	QObject::connect(elementButtons[1], &QPushButton::released, this, &Viewport::setElementEdge);
	QObject::connect(elementButtons[2], &QPushButton::released, this, &Viewport::setElementFace);
	QObject::connect(elementButtons[3], &QPushButton::released, this, &Viewport::setElementObject);

	for (int i = 0; i < 4; ++i) {
		elementButtons[i]->setFixedWidth(64);
		elementButtons[i]->setFixedHeight(64);
		elementButtons[i]->setAutoExclusive(true);
		elementButtons[i]->setCheckable(true);
		elementButtons[i]->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked { background-color: red; }");
	}

	elementButtons[3]->setChecked(true);

	// Right Click Panel Actions

	menuActions[0] = new QAction("Delete", this);
	menuActions[1] = new QAction("Duplicate", this);
	menuActions[2] = new QAction("Orbit Camera", this);

	QObject::connect(menuActions[0], &QAction::triggered, this, &Viewport::deleteSelection);
	QObject::connect(menuActions[1], &QAction::triggered, this, &Viewport::duplicateSelection);
	QObject::connect(menuActions[2], &QAction::triggered, this, &Viewport::orbitSelection);

	// Set Default Settings & Load Default Cube

	scene.NewMesh()->load(CUBE_VERTICES, CUBE_FACES);

	selectionManager.setActiveElement(OBJECT);
	setMouseTracking(true);
}

Viewport::~Viewport()
{}

// ---------------------------------------------------------------------------------------------
// Core OpenGL ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void Viewport::initializeGL() {
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
		this->close();
		return;
	}

	rendererInitialized = true;

	renderer.setCameraPosition(Vec3f(5.0f, 0.0f, 0.0f));
	//renderer.SetVertexPointsVisible(verticesAlwaysVisible);
	renderer.UpdateRenderBuffers(scene);
	renderer.UpdateRenderBuffers();
	renderer.ResetColorBuffer<true>();
	renderer.UpdateUniformBuffer();
	updateSelectionVisuals();
	//renderer.UpdateHover(Vec3u(0u), selectionManager.getActiveElement(), scene);
	updateGizmos();
}

// ---------------------------------------------------------------------------------------------
// Signal Helpers ------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void Viewport::emitPrimarySelectionPosChanged() {
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

void Viewport::emitPrimarySelectionScaleChanged() {
	if (selectionManager.meshCount() != 1 || selectionManager.getActiveElement() != OBJECT) { return; }

	const SelectionItem& item = selectionManager.getSelectedItems()[0];
	Vec3f scale = scene[item.meshId]->Scale();
	emit primarySelectionScaleChanged(scale);
}

void Viewport::emitPrimarySelectionRotChanged() {
	if (selectionManager.meshCount() != 1 || selectionManager.getActiveElement() != OBJECT) { return; }

	const SelectionItem& item = selectionManager.getSelectedItems()[0];
	Vec3f rot = scene[item.meshId]->Rot();
	emit primarySelectionRotChanged(rot);
}

void Viewport::emitSelectionChanged() {
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

// ---------------------------------------------------------------------------------------------
// Event Handlers -------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void Viewport::mousePressEvent(QMouseEvent* event) {
	QOpenGLWidget::mousePressEvent(event);

	if (actionParams.action == EXTRUDE && actionParams.stage == 1) {
		actionParams.stage = 0;
		actionParams.ClearActionDirs();
		actionParams.ClearElements();
		renderer.UpdateRenderBuffers(scene);
		update();
		return;
	}

	// Only accept event if the left mouse button is clicked
	if (!(event->button() & Qt::LeftButton)) {
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
	//const Vec3u ids = renderer.SelectionIds(x, y);
	//const bool activeGizmo =
		//(actionParams.action == TRANSLATE_AXIS && (ids.y == 0 || ids.y == 4)) ||
		//(actionParams.action == SCALE && (ids.y == 1 || ids.y == 4)) ||
		//(actionParams.action == ROTATE && (ids.y == 2 || ids.y == 4));
	//const bool gizmoHit =
		//ids.x == GIZMO_IDENTIFIER && activeGizmo &&
		//((ids.y == 4 && ids.z == 3) || ids.z < 3);
	//draggingGizmo = gizmoHit;
	//draggedGizmoAxis = gizmoHit ? ids.z : MAX_UINT;
}

void Viewport::mouseDoubleClickEvent(QMouseEvent* event) {
	selectionManager.clearSelection();
	makeCurrent();
	updateSelectionVisuals();
	updateGizmos();
	update();

	emit primarySelectionChanged(false);
}

void Viewport::mouseReleaseEvent(QMouseEvent* event) {
	QOpenGLWidget::mouseReleaseEvent(event);

	// Only accept event if the left mouse button is clicked
	if (!(event->button() & Qt::LeftButton)) { return; }

	// Accept the event and make the OpenGL context current in case the renderer needs it for selection behavior
	event->accept();
	makeCurrent();

	// Retrieve the current logical (not physical) pixel position
	Vec2f pos(event->position().x(), this->height() - 1.0f - event->position().y());

	if (actionParams.action == EXTRUDE && actionParams.stage == 1) {
		actionParams.elements.clear();

		Element elementType = selectionManager.getActiveElement();
		if (elementType == OBJECT) { elementType = EDGE; }

		if (elementType == EDGE) { beveler->ExtrudeBeveledEdges(actionParams.elements); }
		else { beveler->ExtrudeBeveledVertices(actionParams.elements); }
		
		uint mesh = selectionManager.getSelectedItems()[0].meshId;
		selectionManager.clearSelection();

		initActionDirsToElementNormals(elementType, mesh);

		mouseAction.MousePress(pos.x, pos.y, this->width(), this->height());
		renderer.UpdateRenderBuffers(scene);
		renderer.ResetColorBuffer<true>();
		update();
		return;
	}

	// Retrieve selection ids from corresponding physical pixel and print them
	//Vec3u ids = renderer.SelectionIds(devicePixelRatio() * pos.x, devicePixelRatio() * (pos.y));

	const bool gizmoDragWasCaptured = draggingGizmo;

	// An axis click is a gizmo interaction, not a mesh selection. The axis
	// was captured on press and must remain captured even if the cursor moved.
	if (!gizmoDragWasCaptured) {
		// Only process selection if the mouse was clicked, not dragged.
		// Also make sure the selection is valid (not background).
		//if (!mouseAction.dragged && ids.x != 0) {
			//handleSelection(ids);
		//}

		// Print selection ids.
		//std::cout << "Object Id: " << ids.x << std::endl;
		//std::cout << "Component Id: " << ids.y << std::endl;
		//std::cout << "Sub-Component Id: " << ids.z << std::endl;
	}

	draggingGizmo = false;
	draggedGizmoAxis = MAX_UINT;

	// A captured gizmo click does not change selection or hover state, so it
	// does not need another repaint on release.
	if (!gizmoDragWasCaptured) {
		update();
	}
}

void Viewport::mouseMoveEvent(QMouseEvent* event) {
	// Accept the event and make the OpenGL context current for renderer.
	event->accept();
	makeCurrent();

	const Vec2f pos(event->position().x(), this->height() - 1.0f - event->position().y());

	// AI-generated Line: Separates passive hover tracking from active drag handling
	// so enabling Qt mouse tracking does not turn ordinary movement into camera movement.
	const bool dragging = event->buttons() & Qt::LeftButton;

	// With mouse tracking enabled, mouseMoveEvent also runs without a button
	// pressed. Hovering must not be interpreted as a camera drag.
	if (dragging) {
		mouseDragEvent(pos);
		return;
	}

	if (actionParams.action == EXTRUDE && actionParams.stage == 1) {
		mouseAction.StartMouseDragStep(pos.x, pos.y, this->width(), this->height());
		translateActionElements();
		mouseAction.EndMouseDragStep();
		renderer.UpdateRenderBuffers(scene);
		update();
		return;
	}

	// Update hover visuals.
	const bool hoverChanged = updateHoverVisual(pos);
	if (hoverChanged) { update(); }

	return;
}

void Viewport::mouseDragEvent(const Vec2f& pos) {
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
	// Old code - currently doesn't do anything
	case TRANSLATE_FREE:
		actionPerformed = translateFreeDrag();
		if (actionPerformed) { emitPrimarySelectionPosChanged(); }
		break;
	case TRANSLATE_EDGE:
		actionPerformed = translateEdgeDrag();
		if (actionPerformed) { emitPrimarySelectionPosChanged(); }
		break;
	case TRANSLATE_AXIS:
		actionPerformed = translateGizmoDrag();
		break;
	case SCALE:
		//actionPerformed = scaleDrag();
		actionPerformed = scaleGizmoDrag();
		if (actionPerformed) { emitPrimarySelectionScaleChanged(); }
		break;
	case ROTATE:
		//actionPerformed = rotateDrag();
		actionPerformed = rotateGizmoDrag();
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

	// If no action performed and no gizmo was captured, move the camera. A
	// captured gizmo must keep ownership of the drag even when its projected
	// axis is degenerate from the current camera angle.
	if (!actionPerformed && !draggingGizmo) {
		renderer.mouseDrag(mouseAction.mouseDragChange);
	}
	else if (actionPerformed && updateRenderBuffers) {
		renderer.UpdateRenderBuffers(scene);
	}

	updateGizmos();

	// Update MouseAction struct
	mouseAction.EndMouseDragStep();

	update();
}

void Viewport::wheelEvent(QWheelEvent* event) {
	// Accept event and get change in wheel
	event->accept();
	QPoint change = event->angleDelta();

	// Convert change in wheel to change in camera
	Vec2f camChange(change.x(), change.y());
	const float SCALE = 0.025f;
	camChange *= SCALE;

	// Update renderer and viewport
	makeCurrent();
	renderer.mouseWheel(camChange.x, camChange.y);

	update();
}

void Viewport::resizeEvent(QResizeEvent* event) {
	QOpenGLWidget::resizeEvent(event);

	const int X_PADDING = 5;
	const int Y_PADDING = 5;

	for (int i = 0; i < 4; ++i) {
		int x = Y_PADDING * (i + 1) + 64 * i;
		int y = this->height() - 1 - Y_PADDING - 64;
		elementButtons[i]->move(x, y);
	}
}

void Viewport::leaveEvent(QEvent* event) {
	event->accept();
	makeCurrent();
	//const bool hoverChanged = renderer.UpdateHover(
		//Vec3u(0u), selectionManager.getActiveElement(), scene);
	const bool gizmoChanged = hoveredGizmoAxis != MAX_UINT;
	hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	hoveredGizmoAxis = MAX_UINT;
	if (gizmoChanged) {
		updateGizmos();
	}
	//if (hoverChanged || gizmoChanged) {
		//update();
	//}
}

void Viewport::contextMenuEvent(QContextMenuEvent* event) {
	if (!(event->reason() == QContextMenuEvent::Mouse)) { 
		event->ignore();
		return; 
	}

	event->accept();

	QMenu menu(this);
	for (int i = 0; i < 3; ++i) {
		menu.addAction(menuActions[i]);
	}

	menu.exec(event->globalPos());
}

// ---------------------------------------------------------------------------------------------
// Misc. Helpers -------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void Viewport::setAction(Action action) {
	actionParams.action = action;
	hoveredGizmoAxis = MAX_UINT;
	draggingGizmo = false;
	draggedGizmoAxis = MAX_UINT;
	if (rendererInitialized) {
		makeCurrent();
		updateGizmos();
	}
	update();
}

// ---------------------------------------------------------------------------------------------
// Selection -----------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void Viewport::handleSelection(const Vec3u& selectionIds) {
	// Create a selection item to send to the selection manager.
	SelectionItem item = createSelectionItem(selectionIds);

	if (item.meshId == MAX_UINT) {
		// Clicking the background is a deselection. Keep the renderer and
		// gizmo in sync when the last selected item is removed.
		if (!selectionManager.getSelectedItems().empty()) {
			selectionManager.clearSelection();
			updateSelectionVisuals();
			updateGizmos();
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
	//updateSelectionVisuals();
	//updateGizmos();

	emitSelectionChanged();
}

void Viewport::setActiveElement(Element element) {
	selectionManager.setActiveElement(element);

	if (!verticesAlwaysVisible) {
		//renderer.SetVertexPointsVisible(element == VERTEX);
	}

	// Changing element mode clears the selection in SelectionManager. Push the
	// empty selection to the renderer immediately so the old highlight vanishes.
	makeCurrent();
	updateSelectionVisuals();
	//renderer.UpdateHover(Vec3u(0u), element, scene);
	hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	hoveredGizmoAxis = MAX_UINT;
	draggingGizmo = false;
	draggedGizmoAxis = MAX_UINT;
	updateGizmos();
	update();

	emit primarySelectionChanged(false);
}

SelectionItem Viewport::createSelectionItem(const Vec3u& ids)
{
	switch (selectionManager.getActiveElement()) {
	case OBJECT:
		// Normal hit.
		if (ids.x != 0 && ids.x < GIZMO_IDENTIFIER) {
			return SelectionItem{
				ids.x - 1,
				MAX_UINT
			};
		}
		// Vertex hit belonging to object.
		else if (ids.x == MAX_UINT && ids.y != 0) {
			return SelectionItem{
				ids.y - 1,
				MAX_UINT
			};
		}

		break;

	case VERTEX:
		if (ids.x == MAX_UINT && ids.y != 0) {
			return SelectionItem{
				ids.y - 1,
				ids.z
			};
		}

		break;

	case FACE:
		if (ids.x != 0 && ids.x < GIZMO_IDENTIFIER && ids.x != MAX_UINT) {
			return SelectionItem{
				ids.x - 1,
				ids.y
			};
		}

		break;

	case EDGE:
		if (ids.x != 0 && ids.x < GIZMO_IDENTIFIER && ids.z != MAX_UINT) {
			return SelectionItem{
				ids.x - 1,
				ids.z
			};
		}

		break;
	}

	return { MAX_UINT, MAX_UINT };
}

void Viewport::updateSelectionVisuals() {
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
	//renderer.UpdateSelection(renderItems, activeElement);
}

// ---------------------------------------------------------------------------------------------
// Drag Actions & Helpers ----------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

bool Viewport::translateFreeDrag() {
	if (selectionManager.getSelectedItems().empty()) { return false; }

	Vec3f f, u, r;
	renderer.GetCamera()->directions(f, u, r);

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

void Viewport::initActionDirsToSelectedNormals() {
	// Get all selected items.
	const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

	bool noValidItems = true;
	actionParams.dirs.assign(selectedItems.size(), Vec3f(0.0f));

	// For each selected item, get normal direction
	for (size_t i = 0; i < selectedItems.size(); ++i) {
		const SelectionItem& item = selectedItems[i];
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		switch (selectionManager.getActiveElement()) {
		case VERTEX:
			actionParams.dirs[i] = scene[item.meshId]->GetVertexNormal(item.componentId);
			break;
		case EDGE:
			actionParams.dirs[i] = scene[item.meshId]->GetEdgeNormal(item.componentId);
			break;
		case FACE:
			actionParams.dirs[i] = scene[item.meshId]->GetFaceNormal(item.componentId);
			break;
		}

		noValidItems = false;
	}

	if (noValidItems) { actionParams.ClearActionDirs(); }
}

void Viewport::initActionDirsToElementNormals(Element elementType, uint mesh) {
	actionParams.dirs.clear();
	actionParams.dirs.reserve(actionParams.elements.size());
	actionParams.elementType = elementType;
	actionParams.mesh = mesh;

	// For each selected item, get normal direction
	for (uint element : actionParams.elements) {
		switch (elementType) {
		case VERTEX:
			actionParams.dirs.push_back(scene[mesh]->GetVertexNormal(element));
			break;
		case EDGE:
			actionParams.dirs.push_back(scene[mesh]->GetEdgeNormal(element));
			break;
		case FACE:
			actionParams.dirs.push_back(scene[mesh]->GetFaceNormal(element));
			break;
		}
	}
}

void Viewport::translateSelections() {
	// Compute translation's scale factor
	const float SCALE_COEF = 0.01f;
	float scale = SCALE_COEF * mouseAction.mouseDragChange.Dot(mouseAction.initialDragDir);

	// Translate selected elements
	const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();
	for (size_t i = 0; i < actionParams.dirs.size(); ++i) {
		const SelectionItem& item = selectedItems[i];
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		switch (selectionManager.getActiveElement()) {
		case VERTEX:
			scene[item.meshId]->TranslateVertex(item.componentId, actionParams.dirs[i] * scale);
			break;
		case EDGE:
			scene[item.meshId]->TranslateEdge(item.componentId, actionParams.dirs[i] * scale);
			break;
		case FACE:
			scene[item.meshId]->TranslateFace(item.componentId, actionParams.dirs[i] * scale);
			break;
		}
	}
}

void Viewport::translateActionElements() {
	// Compute translation's scale factor
	const float SCALE_COEF = 0.01f;
	float scale = SCALE_COEF * mouseAction.mouseDragChange.Dot(mouseAction.initialDragDir);

	for (uint i = 0; i < actionParams.elements.size(); ++i) {
		switch (actionParams.elementType) {
		case VERTEX:
			scene[actionParams.mesh]->TranslateVertex(actionParams.elements[i], actionParams.dirs[i] * scale);
			break;
		case EDGE:
			scene[actionParams.mesh]->TranslateEdge(actionParams.elements[i], actionParams.dirs[i] * scale);
			break;
		case FACE:
			scene[actionParams.mesh]->TranslateFace(actionParams.elements[i], actionParams.dirs[i] * scale);
			break;
		}
	}
}

bool Viewport::translateOutDrag() {
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

	if (actionParams.dirs.empty()) { return false; }

	// Get all selected items.
	const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

	// Translate
	translateSelections();

	return true;
}

bool translateEdgeDrag() { return false; }

void Viewport::setupBevelDrag() {
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
		if (item.meshId != meshId) { continue; }
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

bool Viewport::bevelDrag() {
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

void Viewport::extrudeSelectedElements() {
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) { continue; }

		scene[item.meshId]->Extrude(item.componentId);
	}
}

bool Viewport::extrudeDrag() {
	if (selectionManager.getSelectedItems().empty()) { return false; }

	if (selectionManager.getActiveElement() != FACE) { 
		if (mouseAction.firstMoveEvent) {
			//initActionDirsToSelectedNormals(); // fine as long as edges aren't duplicated
			actionParams.stage = 1;
		}
		return bevelDrag(); 
	}

	// Compute normals if this is the first move event
	if (mouseAction.firstMoveEvent) {
		initActionDirsToSelectedNormals();
	}

	if (actionParams.dirs.empty()) { return false; }

	// Extrude selected elements if this is the first move event
	if (mouseAction.firstMoveEvent) {
		extrudeSelectedElements();
	}

	// Translated extruded faces
	translateSelections();

	return true;
}

bool Viewport::scaleDrag() {
	if (selectionManager.getActiveElement() != OBJECT || selectionManager.getSelectedItems().empty()) {
		return false;
	}

	float factor = 0.01f * mouseAction.mouseDragChange.Dot(mouseAction.initialDragDir);

	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId < scene.MeshCount()) {
			scene[item.meshId]->AddScale(factor);
		}
	}

	//renderer.UpdateRenderBuffers(scene);
	renderer.UpdateUniformBuffer();

	return true;
}

bool Viewport::rotateDrag() {
	if (selectionManager.getActiveElement() != OBJECT || selectionManager.getSelectedItems().empty()) {
		return false;
	}const uint NO_HOVER = 4294967295u;


	Vec3f f, u, r;
	renderer.GetCamera()->directions(f, u, r);

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

	renderer.UpdateUniformBuffer();

	return true;
}

bool Viewport::rotateGizmoDrag() {
	// If we aren't dragging the gizmo, or the active element is not object, or there is nothing selected, do nothing.
	if (!draggingGizmo ||
		selectionManager.getActiveElement() != OBJECT ||
		selectionManager.getSelectedItems().empty()) {
		return false;
	}

	// The center cube performs unconstrained rotation.
	if (draggedGizmoAxis == 3) {
		const bool rotated = rotateDrag();
		if (rotated) {
			emitPrimarySelectionRotChanged();
		}
		return rotated;
	}

	if (draggedGizmoAxis >= 3) {
		return false;
	}

	// Todo: axis rotation.
	bool rotated = false;
	std::cout << "Rotation along axis: " << draggedGizmoAxis << " captured, but logic not implemented." << std::endl;

	renderer.UpdateRenderBuffers(scene);

	if (rotated) {
		emitPrimarySelectionRotChanged();
	}
	return rotated;
}

// ----------------------------------------------------------------------------------------------------
// Gizmos ---------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

// Constrain the drag to the world-space axis captured on mouse press.
bool Viewport::translateGizmoDrag() {
	if (!draggingGizmo ||
		selectionManager.getSelectedItems().empty()) {
		return false;
	}

	// The center cube is free, it should translate free.
	if (draggedGizmoAxis == 3) {
		const bool transformed = translateFreeDrag();
		if (transformed) {
			emitPrimarySelectionPosChanged();
		}
		return transformed;
	}

	// Invalid axis don't do anything.
	if (draggedGizmoAxis >= 3) {
		return false;
	}

	Vec3f forward, up, right;
	renderer.GetCamera()->directions(forward, up, right);

	Vec3f axis;
	switch (draggedGizmoAxis) {
	case 0: axis = Vec3f(1.0f, 0.0f, 0.0f); break;
	case 1: axis = Vec3f(0.0f, 1.0f, 0.0f); break;
	case 2: axis = Vec3f(0.0f, 0.0f, 1.0f); break;
	default: return false;
	}

	// AI-generated: Match axis-drag projection to the rotated gizmo display.
	// We actually want to translate using world space not local space.
	//axis = getGizmoRotation() * axis;
	axis.Normalize();

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

bool Viewport::scaleGizmoDrag() {
	// If we aren't dragging the gizmo, or the active element is not object, or there is nothing selected, do nothing.
	if (!draggingGizmo ||
		selectionManager.getActiveElement() != OBJECT ||
		selectionManager.getSelectedItems().empty()) {
		return false;
	}

	// The center cube should uniform scale.
	if (draggedGizmoAxis == 3) {
		const bool scaled = scaleDrag();
		if (scaled) {
			emitPrimarySelectionPosChanged();
		}
		return scaled;
	}

	if (draggedGizmoAxis >= 3) {
		return false;
	}

	Vec3f forward, up, right;
	renderer.GetCamera()->directions(forward, up, right);

	Vec3f axis;
	switch (draggedGizmoAxis) {
	case 0: axis = Vec3f(1.0f, 0.0f, 0.0f); break;
	case 1: axis = Vec3f(0.0f, 1.0f, 0.0f); break;
	case 2: axis = Vec3f(0.0f, 0.0f, 1.0f); break;
	default: return false;
	}
	// AI-generated: Match scale-axis dragging to the rotated gizmo display.
	// Don't scale by local space axes unless specified.
	if (localScale) axis = getGizmoRotation() * axis;
	axis.Normalize();

	// Project the selected world axis onto the screen to determine how much
	// the mouse moved along it. An axis aimed directly at the camera has no
	// stable screen direction.
	Vec2f axisScreen(axis.Dot(right), axis.Dot(up));
	const float axisScreenLength = axisScreen.Length();
	if (axisScreenLength < 1e-6f) {
		return false;
	}
	axisScreen /= axisScreenLength;

	// Qt coordinates were converted to bottom-left origin in the mouse
	// handlers, so invert the vertical delta to match the FLTK drag math.
	Vec2f change(-mouseAction.mouseDragChange.x, -mouseAction.mouseDragChange.y);
	const float factor = 0.01f * change.Dot(axisScreen);

	bool scaled = false;
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId < scene.MeshCount()) {
			HalfEdgeMesh* mesh = scene[item.meshId];
			const float newScale = std::max(0.001f, mesh->Scale()[draggedGizmoAxis] + factor);
			mesh->Scale(static_cast<int>(draggedGizmoAxis), newScale);
			scaled = true;
		}
	}

	//renderer.UpdateRenderBuffers(scene);

	if (scaled) {
		emitPrimarySelectionScaleChanged();
	}
	return scaled;
}

// AI-generated: Builds the gizmo orientation from the first selected mesh.
Matrix3f Viewport::getGizmoRotation() const {
	Matrix3f rotation;
	rotation.SetIdentity();

	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		const Vec3f& eulerRotation = scene[item.meshId]->Rot();
		rotation.SetRotationZYX(
			degreesToRadians(eulerRotation.x),
			degreesToRadians(eulerRotation.y),
			degreesToRadians(eulerRotation.z));
		break;
	}

	return rotation;
}

// AI-generated code
void Viewport::updateGizmos() {
	Matrix3f gizmoRotation = getGizmoRotation();

	// Update translation gizmo.
	if (actionParams.action == TRANSLATE_AXIS) {
		const bool translationAction = actionParams.action == TRANSLATE_AXIS;
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		// If nothing selected, clear gizmos.
		if (!translationAction || selectedItems.empty()) {
			//renderer.ClearGizmos();
			return;
		}

		// Get the average position of all selected elements.

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

		// If no positions, clear gizmos.
		if (positionCount == 0) {
			//renderer.ClearGizmos();
			return;
		}

		// When translating we want to use the world space rotation, not local.
		gizmoRotation.SetIdentity();

		// Give averaged position and the current highlighted axis to the renderer.
		const uint highlightedAxis = draggingGizmo ? draggedGizmoAxis : hoveredGizmoAxis;
		//renderer.UpdateGizmos(positionSum / float(positionCount), highlightedAxis, GizmoRenderer::Type::Translation, gizmoRotation);
	}
	// Update scale gizmo.
	else if (actionParams.action == SCALE) {
		const bool scaleAction = actionParams.action == SCALE;
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		// If nothing selected, or the active element is not objects, clear gizmos.
		if (!scaleAction || selectionManager.getActiveElement() != OBJECT ||
			selectedItems.empty()) {
			//renderer.ClearGizmos();
			return;
		}

		// Get the average position of all selected elements, only objects as scale is only defined for objects.

		Vec3f positionSum(0.0f);
		uint positionCount = 0;

		for (const SelectionItem& item : selectedItems) {
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			HalfEdgeMesh* mesh = scene[item.meshId];
			Vec3f worldPosition = mesh->Pos();

			positionSum += worldPosition;
			++positionCount;
		}

		// If no positions, clear gizmos.
		if (positionCount == 0) {
			//renderer.ClearGizmos();
			return;
		}

		// When scaling we want to use the world space rotation, not local.
		if (!localScale) gizmoRotation.SetIdentity();

		// Give averaged position and the current highlighted axis to the renderer.
		const uint highlightedAxis = draggingGizmo ? draggedGizmoAxis : hoveredGizmoAxis;
		//renderer.UpdateGizmos(positionSum / float(positionCount), highlightedAxis, GizmoRenderer::Type::Scale, gizmoRotation);
	}
	// Update rotate gizmo.
	else if (actionParams.action == ROTATE) {
		const bool rotateAction = actionParams.action == ROTATE;
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		// If nothing selected, or the active element is not objects, clear gizmos.
		if (!rotateAction || selectionManager.getActiveElement() != OBJECT ||
			selectedItems.empty()) {
			//renderer.ClearGizmos();
			return;
		}

		// Get the average position of all selected elements, only objects as rotate is only defined for objects.

		Vec3f positionSum(0.0f);
		uint positionCount = 0;

		for (const SelectionItem& item : selectedItems) {
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			HalfEdgeMesh* mesh = scene[item.meshId];
			Vec3f worldPosition = mesh->Pos();

			positionSum += worldPosition;
			++positionCount;
		}

		// If no positions, clear gizmos.
		if (positionCount == 0) {
			//renderer.ClearGizmos();
			return;
		}

		// Give averaged position and the current highlighted axis to the renderer.
		const uint highlightedAxis = draggingGizmo ? draggedGizmoAxis : hoveredGizmoAxis;
		//renderer.UpdateGizmos(positionSum / float(positionCount), highlightedAxis, GizmoRenderer::Type::Rotation, gizmoRotation);
	}
	// Else, undefined gizmo, clear all gizmos.
	else {
		//renderer.ClearGizmos();
	}
}

// -------------------------------------------------------------------------------------------------
// Hover Visuals -----------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

// AI Generated Code
bool Viewport::updateHoverVisual(const Vec2f& pos) {
	const float pixelRatio = static_cast<float>(devicePixelRatio());
	const int pixelWidth = std::max(1, static_cast<int>(width() * pixelRatio));
	const int pixelHeight = std::max(1, static_cast<int>(height() * pixelRatio));
	const int rawX = static_cast<int>(pos.x * pixelRatio);
	const int rawY = static_cast<int>(pos.y * pixelRatio);
	const uint x = static_cast<uint>(std::clamp(rawX, 0, pixelWidth - 1));
	const uint y = static_cast<uint>(std::clamp(rawY, 0, pixelHeight - 1));
	//const Vec3u ids = renderer.SelectionIds(x, y);
	//const bool activeGizmo =
		//(actionParams.action == TRANSLATE_AXIS && (ids.y == 0 || ids.y == 4)) ||
		//(actionParams.action == SCALE && (ids.y == 1 || ids.y == 4)) ||
		//(actionParams.action == ROTATE && (ids.y == 2 || ids.y == 4));
	//const uint newHoveredAxis =
		//(ids.x == GIZMO_IDENTIFIER && activeGizmo &&
			//((ids.y == 4 && ids.z == 3) || ids.z < 3))
		//? ids.z
		//: MAX_UINT;

	//const bool gizmoChanged = newHoveredAxis != hoveredGizmoAxis;
	//hoveredSelectionIds = ids;
	//hoveredGizmoAxis = newHoveredAxis;

	//const bool rendererChanged = renderer.UpdateHover(
		//ids, selectionManager.getActiveElement(), scene);
	//if (gizmoChanged) {
		//updateGizmos();
	//}
	// Raw framebuffer IDs can change while the same logical object remains
	// hovered (for example, crossing faces in OBJECT mode). Only repaint when
	// the logical mesh hover or gizmo hover actually changes.
	//return gizmoChanged || rendererChanged;
	return false;
}

