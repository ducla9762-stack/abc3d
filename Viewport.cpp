#include "Viewport.h"
#include "shapes.h"
#include <qpushbutton.h>
#include <QMenu>
#include <QPainter>
#include <unordered_map>
#include <unordered_set>

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
	elementButtons[0] = new QPushButton(QIcon("assets/icons/vertex.svg"), "", this);
	elementButtons[1] = new QPushButton(QIcon("assets/icons/edge.svg"), "", this);
	elementButtons[2] = new QPushButton(QIcon("assets/icons/face.svg"), "", this);
	elementButtons[3] = new QPushButton(QIcon("assets/icons/object.svg"), "", this);

	elementButtons[0]->setToolTip("<b> Vertex Selection </b> <br> '1' Key shortcut");
	elementButtons[1]->setToolTip("<b> Edge Selection </b> <br> '2' Key shortcut");
	elementButtons[2]->setToolTip("<b> Face Selection </b> <br> '3' Key shortcut");
	elementButtons[3]->setToolTip("<b> Object Selection </b> <br> '4' Key shortcut");

	QObject::connect(elementButtons[0], &QPushButton::released, this, &Viewport::setElementVertex);
	QObject::connect(elementButtons[1], &QPushButton::released, this, &Viewport::setElementEdge);
	QObject::connect(elementButtons[2], &QPushButton::released, this, &Viewport::setElementFace);
	QObject::connect(elementButtons[3], &QPushButton::released, this, &Viewport::setElementObject);


	for (int i = 0; i < 4; ++i) {
		elementButtons[i]->setIconSize(QSize(64, 64));
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

	knifeOverlay = new OverlayWidget(this);
	knifeOverlay->draw = [this](QPainter& painter) { drawKnifeOverlay(painter); };
	knifeOverlay->hide();
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

	renderer.setCameraPosition(Vec3f(5.0f, 0.0f, 0.0f));
	renderer.SetScene(scene);
	renderer.UpdateGeometry();
	renderer.UpdateTransforms();
	renderer.UpdateSelection(selectionManager);
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
// Event Handlers ------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void Viewport::mousePressEvent(QMouseEvent* event) {
	QOpenGLWidget::mousePressEvent(event);

	if (actionParams.action == EXTRUDE && actionParams.stage == 1) {
		actionParams.stage = 0;
		actionParams.ClearActionDirs();
		actionParams.ClearElements();
		return;
	}

	// Only accept event if the left mouse button is clicked
	if (!(event->button() & Qt::LeftButton)) {
		return;
	}

	// Accept the event and make the OpenGL context current in case the renderer needs it for selection behavior
	event->accept();
	makeCurrent();

	// An edge choice belongs to one mouse drag.  Do not carry a previously
	// chosen half-edge into the next drag (or into a changed selection).
	edgeDragEdges.clear();
	edgeDragDistances.clear();

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
	const Vec3u ids = renderer.GizmoSelectionIds(x, y);
	const bool activeGizmo =
		(actionParams.action == TRANSLATE_AXIS && (ids.y == 0 || ids.y == 4)) ||
		(actionParams.action == SCALE && (ids.y == 1 || ids.y == 4)) ||
		(actionParams.action == ROTATE && (ids.y == 2 || ids.y == 4));
	const bool gizmoHit =
		ids.x == GIZMO_IDENTIFIER && activeGizmo &&
		((ids.y == 4 && ids.z == 3) || ids.z < 3);
	draggingGizmo = gizmoHit;
	draggedGizmoAxis = gizmoHit ? ids.z : MAX_UINT;
	draggedRotationAxis = Vec3f(0.0f);
	if (gizmoHit && actionParams.action == ROTATE && ids.z < 3) {
		Vec3f localAxis(0.0f);
		if (ids.z == 0) localAxis.x = 1.0f;
		else if (ids.z == 1) localAxis.y = 1.0f;
		else if (ids.z == 2) localAxis.z = 1.0f;
		draggedRotationAxis = getGizmoRotation() * localAxis;
		draggedRotationAxis.Normalize();
	}
}

void Viewport::mouseDoubleClickEvent(QMouseEvent* event) {
	// The second click of a quick double click is a knife point (added on release), not a deselect
	if (actionParams.action == KNIFE) { event->accept(); return; }
	if (actionParams.action == MERGE) {
		mergeFirstVertex.reset();
	}
	selectionManager.clearSelection();
	renderer.UpdateSelection(selectionManager);
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

	// A click (not a camera drag) adds a point to the cut
	if (actionParams.action == KNIFE) {
		if (!mouseAction.dragged) {
			knifeCursor = knifeToNdc(event->position());
			knifeHasCursor = true;
			knifePath.push_back(knifeCursor);
			knifeRecompute();
		}
		update();
		return;
	}

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
		renderer.UpdateGeometry();
		renderer.UpdateSelection(selectionManager);
		update();
		return;
	}

	// Retrieve selection ids from corresponding physical pixel and print them
	Vec2u ids = renderer.SelectionIds(selectionManager.getActiveElement(), devicePixelRatio() * pos.x, devicePixelRatio() * (pos.y));

	const bool gizmoDragWasCaptured = draggingGizmo;

	// An axis click is a gizmo interaction, not a mesh selection. The axis
	// was captured on press and must remain captured even if the cursor moved.
	if (!gizmoDragWasCaptured) {
		// Only process selection if the mouse was clicked, not dragged.
		// Also make sure the selection is valid (not background).
		if (!mouseAction.dragged) {
			if (actionParams.action == MERGE) {
				handleMergeVertexClick(ids);
			}
			else if (actionParams.action == COLLAPSE) {
				handleCollapseEdgeClick(ids);
			}
			else if (actionParams.action == DISSOLVE) {
				handleDissolveEdgeClick(ids);
			}
			else {
				handleSelection(ids);
			}
		}

		// Print selection ids.
		std::cout << "Object Id: " << ids.x << std::endl;
		std::cout << "Element Id: " << ids.y << std::endl;
	}

	draggingGizmo = false;
	draggedGizmoAxis = MAX_UINT;
	draggedRotationAxis = Vec3f(0.0f);
	updateGizmos();

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

	if (actionParams.action == KNIFE) {
		knifeCursor = knifeToNdc(event->position());
		knifeHasCursor = true;
		knifeRecompute();
		update();
		return;
	}

	if (actionParams.action == EXTRUDE && actionParams.stage == 1) {
		mouseAction.StartMouseDragStep(pos.x, pos.y, this->width(), this->height());
		translateActionElements();
		mouseAction.EndMouseDragStep();
		renderer.UpdateGeometry();
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
	//bool updateRenderBuffers = true;

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
		if (actionPerformed) { emitPrimarySelectionPosChanged(); }
		break;
	case SCALE:
		actionPerformed = scaleGizmoDrag();
		if (actionPerformed) { emitPrimarySelectionScaleChanged(); }
		break;
	case ROTATE:
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
		//updateRenderBuffers = false;
		break;
	}

	// If no action performed and no gizmo was captured, move the camera. A
	// captured gizmo must keep ownership of the drag even when its projected
	// axis is degenerate from the current camera angle.
	if (!actionPerformed && !draggingGizmo) {
		renderer.mouseDrag(mouseAction.mouseDragChange);
		if (renderer.GetActiveCamType() == CameraType::FREE) {
			emit primaryCamParamChanged(renderer.getPrimaryCamParam());
		}
		knifeRecompute(); // the path stays put on the screen, so what it hits changes
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
	renderer.mouseWheel(camChange.x, camChange.y);

	if (renderer.GetActiveCamType() == CameraType::FREE) {
		emit primaryCamParamChanged(renderer.getPrimaryCamParam());
	}

	knifeRecompute();

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
	const bool gizmoChanged = hoveredGizmoAxis != MAX_UINT;
	hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	hoveredGizmoAxis = MAX_UINT;
	renderer.ClearHover();
	if (gizmoChanged) updateGizmos();
	update();
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
	if (actionParams.action == KNIFE && action != KNIFE) { knifeFinish(); }
	if ((actionParams.action == MERGE && action != MERGE) ||
		(actionParams.action == COLLAPSE && action != COLLAPSE) ||
		(actionParams.action == DISSOLVE && action != DISSOLVE)) {
		if (actionParams.action == MERGE) {
			mergeFirstVertex.reset();
		}
		selectionManager.clearSelection();
		enableAllElements();
	}

	actionParams.action = action;
	// The toolbar's "Free" mode uses TRANSLATE_AXIS so it can display the
	// active translation gizmo. Both free-translation actions allow every
	// element type, including objects.
	if (action == TRANSLATE_FREE || action == TRANSLATE_AXIS) {
		enableAllElements();
	}

	else if (action == TRANSLATE_OUT) {
		enableElementVertex();
		enableElementEdge();
		enableElementFace();
		disableElementObject();
	}

	else if (action == TRANSLATE_EDGE) {
		enableElementVertex();
		disableElementEdge();
		disableElementFace();
		disableElementObject();
	}

	

	if (action == MERGE) {
		enableElementVertex();
		disableElementEdge();
		disableElementFace();
		disableElementObject();
	}
	else if (action == COLLAPSE) {
		enableElementEdge();
		disableElementVertex();
		disableElementFace();
		disableElementObject();
	}
	else if (action == DISSOLVE) {
		enableElementEdge();
		disableElementVertex();
		disableElementFace();
		disableElementObject();
	}
	if (action == KNIFE) { knifeBegin(); }
	hoveredGizmoAxis = MAX_UINT;
	draggingGizmo = false;
	draggedGizmoAxis = MAX_UINT;
	draggedRotationAxis = Vec3f(0.0f);
	if (renderer.Initialized()) {
		updateGizmos();
		update();
	}
}

// ---------------------------------------------------------------------------------------------
// Selection -----------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------


void Viewport::handleMergeVertexClick(const Vec2u& selectionIds) {
	// SelectionIds() returns a zero-based mesh ID and local vertex ID.
	if (selectionIds.x == MAX_UINT || selectionIds.y == MAX_UINT) {
		return;
	}

	const uint meshId = selectionIds.x;
	const uint vertexId = selectionIds.y;
	if (meshId >= scene.MeshCount() || vertexId >= scene[meshId]->VertexCount()) {
		return;
	}

	const SelectionItem clicked{ meshId, vertexId };
	std::cout << "Merge mode vertex clicked: mesh " << meshId
		<< ", vertex " << vertexId << std::endl;

	if (!mergeFirstVertex.has_value()) {
		mergeFirstVertex = clicked;
		selectionManager.clearSelection();
		selectionManager.addSelection(clicked);
		renderer.UpdateSelection(selectionManager);
		renderer.ClearHover();
		emitSelectionChanged();
		update();
		return;
	}

	const SelectionItem source = *mergeFirstVertex;
	if (source == clicked) {
		std::cout << "Merge rejected: choose a different target vertex." << std::endl;
		return;
	}
	if (source.meshId != clicked.meshId) {
		std::cout << "Merge rejected: both vertices must belong to the same mesh." << std::endl;
		return;
	}

	if (!scene[clicked.meshId]->MergeVertices(source.componentId, clicked.componentId)) {
		std::cout << "Merge rejected: the result would have invalid topology." << std::endl;
		return;
	}

	std::cout << "Merged vertex " << source.componentId << " into vertex "
		<< clicked.componentId << " on mesh " << clicked.meshId << std::endl;
	mergeFirstVertex.reset();
	selectionManager.clearSelection();
	renderer.UpdateGeometry();
	renderer.UpdateSelection(selectionManager);
	renderer.ClearHover();
	hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	emitSelectionChanged();
	update();
}

void Viewport::handleCollapseEdgeClick(const Vec2u& selectionIds) {
	// SelectionIds() returns a zero-based mesh ID and local edge ID.
	if (selectionIds.x == MAX_UINT || selectionIds.y == MAX_UINT) {
		return;
	}

	const uint meshId = selectionIds.x;
	const uint edgeId = selectionIds.y;
	if (meshId >= scene.MeshCount() || edgeId >= scene[meshId]->EdgeCount()) {
		return;
	}

	std::cout << "Collapse mode edge clicked: mesh " << meshId
		<< ", edge " << edgeId << std::endl;

	if (!scene[meshId]->CollapseEdge(edgeId)) {
		std::cout << "Edge collapse rejected: the result would have invalid or open topology."
			<< std::endl;
		return;
	}

	std::cout << "Collapsed edge " << edgeId << " on mesh " << meshId << std::endl;
	selectionManager.clearSelection();
	renderer.UpdateGeometry();
	renderer.UpdateSelection(selectionManager);
	renderer.ClearHover();
	hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	emitSelectionChanged();
	update();
}

void Viewport::handleDissolveEdgeClick(const Vec2u& selectionIds) {
	// SelectionIds() returns a zero-based mesh ID and local edge ID.
	if (selectionIds.x == MAX_UINT || selectionIds.y == MAX_UINT) {
		return;
	}

	const uint meshId = selectionIds.x;
	const uint edgeId = selectionIds.y;
	if (meshId >= scene.MeshCount() || edgeId >= scene[meshId]->EdgeCount()) {
		return;
	}

	std::cout << "Dissolve mode edge clicked: mesh " << meshId
		<< ", edge " << edgeId << std::endl;

	if (!scene[meshId]->DissolveEdge(edgeId)) {
		std::cout << "Dissolve rejected: choose an interior edge with valid topology."
			<< std::endl;
		return;
	}

	std::cout << "Dissolved edge " << edgeId << " on mesh " << meshId << std::endl;
	selectionManager.clearSelection();
	renderer.UpdateGeometry();
	renderer.UpdateSelection(selectionManager);
	renderer.ClearHover();
	hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	emitSelectionChanged();
	update();
}

void Viewport::handleSelection(const Vec2u& selectionIds) {
	// Create a selection item to send to the selection manager.
	SelectionItem item = createSelectionItem(selectionIds);

	if (item.meshId == MAX_UINT) {
		// Clicking the background is a deselection. Keep the renderer and
		// gizmo in sync when the last selected item is removed.
		if (!selectionManager.getSelectedItems().empty()) {
			selectionManager.clearSelection();
			renderer.UpdateSelection(selectionManager);
			updateGizmos();
			emitSelectionChanged();
		}
		return;
	}

	selectionManager.processSelection(item);

	// Tell the renderer to update the selection visuals.
	renderer.UpdateSelection(selectionManager);
	updateGizmos();

	emitSelectionChanged();
}

Element Viewport::getActiveElement() {
	return selectionManager.getActiveElement();
}

void Viewport::setActiveElement(Element element) {
	selectionManager.setActiveElement(element);

	if (element == VERTEX) {
		if (!renderer.ShowVertices()) { renderer.ShowVertices(true); }
	}
	else if(!verticesAlwaysVisible) {
		renderer.ShowVertices(false);
	}

	// Changing element mode clears the selection in SelectionManager. Push the
	// empty selection to the renderer immediately so the old highlight vanishes.
	//renderer.UpdateHover(Vec3u(0u), element, scene);
	renderer.UpdateSelection(selectionManager);
	hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	hoveredGizmoAxis = MAX_UINT;
	draggingGizmo = false;
	draggedGizmoAxis = MAX_UINT;
	renderer.ClearHover();
	updateGizmos();
	update();

	emit primarySelectionChanged(false);
}

SelectionItem Viewport::createSelectionItem(const Vec2u& ids)
{
	if (ids.y == MAX_UINT) { return { MAX_UINT, MAX_UINT }; }
	if (selectionManager.getActiveElement() == OBJECT) {
		return{ ids.x, ids.x };
	}
	
	return { ids.x, ids.y };
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

	if (transformed) {
		if (selectionManager.getActiveElement() == OBJECT) {
			renderer.UpdateTransforms();
		}
		else {
			renderer.UpdateGeometry();
		}
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

	renderer.UpdateGeometry();

	return true;
}

// AI-generated Code
bool Viewport::translateEdgeDrag() {
	// Edge translation is defined for vertices: the vertex remains selected and
	// moves along one of its incident edges.  The edge is selected from the
	// direction of the first drag movement.
	if (selectionManager.getActiveElement() != VERTEX ||
		selectionManager.getSelectedItems().size() != 1) {
		return false;
	}

	const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

	if (mouseAction.firstMoveEvent) {
		Vec3f forward, up, right;
		renderer.GetCamera()->directions(forward, up, right);

		// Mouse positions are converted to a bottom-left origin in the Qt event
		// handlers. Camera::directions() uses the opposite-handed basis from the
		// view matrix (its right/up vectors point toward screen-left/down), so
		// negate both axes before comparing projected edges to the mouse vector.
		const Vec2f& dragDir = mouseAction.initialDragDir;
		if (dragDir.LengthSquared() < 1e-12f) {
			return false;
		}

		edgeDragEdges.clear();
		edgeDragDistances.clear();
		for (const SelectionItem& item : selectedItems) {
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			HalfEdgeMesh* mesh = scene[item.meshId];
			const uint edge = mesh->PickOutgoingHalfEdge(
				item.componentId, dragDir, -right, -up);
			if (edge != MAX_UINT) {
				edgeDragEdges.emplace(item, edge);
				edgeDragDistances.emplace(item, 0.0f);
			}
		}
	}

	if (edgeDragEdges.empty()) {
		return false;
	}

	const float delta = 0.01f * mouseAction.mouseDragChange.Dot(mouseAction.initialDragDir);
	bool transformed = false;
	for (const SelectionItem& item : selectedItems) {
		auto edge = edgeDragEdges.find(item);
		auto distance = edgeDragDistances.find(item);
		if (edge == edgeDragEdges.end() || distance == edgeDragDistances.end() ||
			item.meshId >= scene.MeshCount()) {
			continue;
		}

		// Do not allow a reversed drag to carry the vertex past its starting
		// position. Keep reporting the drag as handled at the limit so the
		// camera does not start orbiting when the clamp is reached.
		const float nextDistance = std::max(0.0f, distance->second + delta);
		const float appliedDistance = nextDistance - distance->second;
		distance->second = nextDistance;
		if (std::abs(appliedDistance) > 1e-12f) {
			scene[item.meshId]->TranslateVertexAlongEdge(
				item.componentId, edge->second, appliedDistance);
		}
		transformed = true;
	}

	if (transformed) {
		renderer.UpdateGeometry();
	}

	return transformed;
}

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

	renderer.UpdateSelection(selectionManager);

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

	renderer.UpdateGeometry();

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
		renderer.UpdateSelection(selectionManager);
	}

	// Translated extruded faces
	translateSelections();

	renderer.UpdateGeometry();

	return true;
}

bool Viewport::scaleDrag() {
	if (selectionManager.getSelectedItems().empty()) {
		return false;
	}

	float factor = 0.01f * mouseAction.mouseDragChange.Dot(mouseAction.initialDragDir);
	const Element activeElement = selectionManager.getActiveElement();

	if (activeElement == EDGE || activeElement == FACE) {
		const bool scaled = scaleSelectedComponents(factor, 0, true);
		if (scaled) {
			renderer.UpdateGeometry();
		}
		return scaled;
	}

	if (activeElement == VERTEX) {
		return false;
	}

	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId < scene.MeshCount()) {
			scene[item.meshId]->AddScale(factor);
		}
	}

	renderer.UpdateTransforms();

	return true;
}

bool Viewport::scaleSelectedComponents(float amount, uint axis, bool uniform) {
	const Element activeElement = selectionManager.getActiveElement();
	// If the active element is not edge or face, or there is no selection, do nothing.
	if ((activeElement != EDGE && activeElement != FACE) ||
		selectionManager.getSelectedItems().empty() || (!uniform && axis >= 3)) {
		return false;
	}

	// Collect all relevant vertices, pivots, and the number of pivots.
	std::unordered_map<uint, std::unordered_set<uint>> selectedVertices;
	std::unordered_map<uint, Vec3f> pivotSums;
	std::unordered_map<uint, uint> pivotCounts;

	// Loop through each selected item.
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) {
			continue;
		}

		// Get the mesh, and collect the vertices that are within the selection.
		HalfEdgeMesh* mesh = scene[item.meshId];
		std::unordered_set<uint>& vertices = selectedVertices[item.meshId];
		std::vector<uint> componentVertices;
		// Init pivot sum.
		if (!pivotCounts.count(item.meshId)) {
			pivotSums[item.meshId] = Vec3f(0.0f);
		}

		// Collect edge vertices, calculate pivots.
		if (activeElement == EDGE) {
			componentVertices.reserve(2);
			mesh->GetEdgeVertices(item.componentId, componentVertices);
			pivotSums[item.meshId] += mesh->GetEdgeCenter(item.componentId);
		}
		// Collect face vertices, calculate pivots.
		else {
			mesh->GetFaceVertices(item.componentId, componentVertices);
			pivotSums[item.meshId] += mesh->faceCentroid(item.componentId);
		}
		vertices.insert(componentVertices.begin(), componentVertices.end());
		++pivotCounts[item.meshId];
	}

	if (selectedVertices.empty()) {
		return false;
	}

	// Convert mouse movement into scale multiplier.
	const float multiplier = std::max(0.001f, 1.0f + amount);
	// Get scale factors, if uniform ignore axis.
	Vec3f factors(uniform ? multiplier : 1.0f);
	if (!uniform) {
		factors[axis] = multiplier;
	}

	// Scale all the relevant vertices by factor.
	for (auto& [meshId, vertices] : selectedVertices) {
		HalfEdgeMesh* mesh = scene[meshId];
		std::vector<uint> vertexIds(vertices.begin(), vertices.end());
		const Vec3f pivot = pivotSums[meshId] / float(pivotCounts[meshId]);
		mesh->ScaleVertices(vertexIds, pivot, factors);
	}

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

	renderer.UpdateTransforms();

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

	if (draggedRotationAxis.LengthSquared() <= 1e-8f) {
		return false;
	}

	Vec3f forward, up, right;
	renderer.GetCamera()->directions(forward, up, right);
	Vec3f screenTangent = draggedRotationAxis.Cross(forward);
	if (screenTangent.LengthSquared() <= 1e-8f) {
		screenTangent = draggedRotationAxis.Cross(up);
	}
	if (screenTangent.LengthSquared() <= 1e-8f) {
		return false;
	}
	screenTangent.Normalize();
	const Vec2f tangentScreen(screenTangent.Dot(right), screenTangent.Dot(up));
	// Invert the screen-drag direction so the mesh follows the gizmo handle.
	const float angle = -0.01f * Vec2f(-mouseAction.mouseDragChange.x,
		-mouseAction.mouseDragChange.y).Dot(tangentScreen);
	if (std::abs(angle) <= 1e-8f) {
		return false;
	}

	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId >= scene.MeshCount()) continue;
		const Vec3f euler = scene[item.meshId]->Rot();
		Matrix3f original;
		original.SetRotationZYX(degreesToRadians(euler.x), degreesToRadians(euler.y), degreesToRadians(euler.z));
		Matrix3f delta;
		delta.SetRotation(draggedRotationAxis, angle);
		delta *= original;
		scene[item.meshId]->Rot(matrixToEulerAngle(delta));
	}
	renderer.UpdateTransforms();
	emitPrimarySelectionRotChanged();
	return true;
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

	// Translation gizmos use world-space axes rather than the selected object's
	// local orientation.
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
		if (selectionManager.getActiveElement() == OBJECT) renderer.UpdateTransforms();
		else renderer.UpdateGeometry();
		emitPrimarySelectionPosChanged();
	}
	return transformed;
}

bool Viewport::scaleGizmoDrag() {
	// Do nothing if we aren't dragging a gizmo, if vertices are the active element, or have no selection.
	if (!draggingGizmo ||
		selectionManager.getActiveElement() == VERTEX ||
		selectionManager.getSelectedItems().empty()) {
		return false;
	}

	// The center cube should uniform scale.
	if (draggedGizmoAxis == 3) {
		const bool scaled = scaleDrag();
		if (scaled) {
			emitPrimarySelectionScaleChanged();
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
	const Element activeElement = selectionManager.getActiveElement();
	// If we are scaling edges or faces
	if (activeElement == EDGE || activeElement == FACE) {
		if (!localScale) {
			return false;
		}
		scaled = scaleSelectedComponents(factor, draggedGizmoAxis, false);
	}
	// Else we are scaling meshes.
	else {
		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId < scene.MeshCount()) {
				HalfEdgeMesh* mesh = scene[item.meshId];
				const float newScale = std::max(0.001f, mesh->Scale()[draggedGizmoAxis] + factor);
				mesh->Scale(static_cast<int>(draggedGizmoAxis), newScale);
				scaled = true;
			}
		}
	}

	if (scaled) {
		if (activeElement == OBJECT) renderer.UpdateTransforms();
		else renderer.UpdateGeometry();
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
			renderer.ClearGizmos();
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
			renderer.ClearGizmos();
			return;
		}

		// When translating we want to use the world space rotation, not local.
		gizmoRotation.SetIdentity();

		// Give averaged position and the current highlighted axis to the renderer.
		const uint highlightedAxis = draggingGizmo ? draggedGizmoAxis : hoveredGizmoAxis;
		renderer.UpdateGizmos(positionSum / float(positionCount), highlightedAxis,
			GizmoRenderer::Type::Translation, gizmoRotation);
	}
	// Update scale gizmo.
	else if (actionParams.action == SCALE) {
		const bool scaleAction = actionParams.action == SCALE;
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		// A single vertex has no extent to scale, so only that element mode omits
		// the scale gizmo.
		const Element activeElement = selectionManager.getActiveElement();
		const bool componentWorldScale =
			(activeElement == EDGE || activeElement == FACE) && !localScale;
		if (!scaleAction || activeElement == VERTEX || componentWorldScale ||
			selectedItems.empty()) {
			renderer.ClearGizmos();
			return;
		}

		// Use the average selected-element center as the shared scale pivot.

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
				break;
			case EDGE:
				worldPosition = mesh->LocalPosToWorldPos(mesh->GetEdgeCenter(item.componentId));
				break;
			case FACE:
				worldPosition = mesh->LocalPosToWorldPos(mesh->faceCentroid(item.componentId));
				break;
			default:
				continue;
			}

			positionSum += worldPosition;
			++positionCount;
		}

		// If no positions, clear gizmos.
		if (positionCount == 0) {
			renderer.ClearGizmos();
			return;
		}

		// When scaling we want to use the world space rotation, not local.
		if (!localScale) gizmoRotation.SetIdentity();

		// Give averaged position and the current highlighted axis to the renderer.
		const uint highlightedAxis = draggingGizmo ? draggedGizmoAxis : hoveredGizmoAxis;
		renderer.UpdateGizmos(positionSum / float(positionCount), highlightedAxis,
			GizmoRenderer::Type::Scale, gizmoRotation);
	}
	// Update rotate gizmo.
	else if (actionParams.action == ROTATE) {
		const bool rotateAction = actionParams.action == ROTATE;
		const std::vector<SelectionItem>& selectedItems = selectionManager.getSelectedItems();

		// If nothing selected, or the active element is not objects, clear gizmos.
		if (!rotateAction || selectionManager.getActiveElement() != OBJECT ||
			selectedItems.empty()) {
			renderer.ClearGizmos();
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
			renderer.ClearGizmos();
			return;
		}

		// Give averaged position and the current highlighted axis to the renderer.
		const uint highlightedAxis = draggingGizmo ? draggedGizmoAxis : hoveredGizmoAxis;
		renderer.UpdateGizmos(positionSum / float(positionCount), highlightedAxis,
			GizmoRenderer::Type::Rotation, gizmoRotation);
	}
	// Else, undefined gizmo, clear all gizmos.
	else {
		renderer.ClearGizmos();
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
	const uint x = static_cast<uint>(std::clamp(static_cast<int>(pos.x * pixelRatio), 0, pixelWidth - 1));
	const uint y = static_cast<uint>(std::clamp(static_cast<int>(pos.y * pixelRatio), 0, pixelHeight - 1));

	const Vec3u gizmoIds = renderer.GizmoSelectionIds(x, y);
	const bool activeGizmo =
		(actionParams.action == TRANSLATE_AXIS && (gizmoIds.y == 0 || gizmoIds.y == 4)) ||
		(actionParams.action == SCALE && (gizmoIds.y == 1 || gizmoIds.y == 4)) ||
		(actionParams.action == ROTATE && (gizmoIds.y == 2 || gizmoIds.y == 4));
	const uint newHoveredAxis = gizmoIds.x == GIZMO_IDENTIFIER && activeGizmo &&
		((gizmoIds.y == 4 && gizmoIds.z == 3) || gizmoIds.z < 3)
		? gizmoIds.z : MAX_UINT;
	const bool gizmoChanged = newHoveredAxis != hoveredGizmoAxis;
	hoveredGizmoAxis = newHoveredAxis;
	if (gizmoChanged) { updateGizmos(); }

	const bool gizmoHit = gizmoIds.x == GIZMO_IDENTIFIER && activeGizmo &&
		((gizmoIds.y == 4 && gizmoIds.z == 3) || gizmoIds.z < 3);
	const Vec2u selectionIds = gizmoHit
		? Vec2u(MAX_UINT)
		: renderer.SelectionIds(selectionManager.getActiveElement(), x, y);
	const Vec2u oldHover(hoveredSelectionIds.x, hoveredSelectionIds.y);
	const bool meshChanged = selectionIds.x != oldHover.x || selectionIds.y != oldHover.y;
	hoveredSelectionIds = Vec3u(selectionIds.x, selectionIds.y, 0);
	if (meshChanged) {
		if (selectionIds.x == MAX_UINT || selectionIds.y == MAX_UINT) {
			renderer.ClearHover();
		}
		else {
			renderer.UpdateHover(selectionIds.x, selectionIds.y);
		}
	}
	return gizmoChanged || meshChanged;
}
// ---------------------------------------------------------------------------------------------
// Knife ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

Vec2f Viewport::knifeToNdc(const QPointF& pos) const {
	return Vec2f(
		2.0f * static_cast<float>(pos.x()) / std::max(1, width()) - 1.0f,
		1.0f - 2.0f * static_cast<float>(pos.y()) / std::max(1, height()));
}

void Viewport::knifeBegin() {
	knifeSession = true;
	knifePath.clear();
	knifeHasCursor = false;
	knifeOriginals.clear();
	knifeOccluders = KnifeOccluders();

	// Only selected meshes are cut, but any mesh can hide them
	std::unordered_set<uint> selected;
	for (const SelectionItem& item : selectionManager.getSelectedItems()) {
		if (item.meshId < scene.MeshCount() && selected.insert(item.meshId).second) {
			knifeOriginals.emplace_back(item.meshId, *scene[item.meshId]);
		}
	}
	for (uint i = 0; i < scene.MeshCount(); ++i) { knifeOccluders.Add(*scene[i]); }
}

void Viewport::knifeRestoreOriginals() {
	for (auto& original : knifeOriginals) {
		if (original.first < scene.MeshCount()) { *scene[original.first] = original.second; }
	}
}

void Viewport::knifeRecompute() {
	if (!knifeSession || !renderer.Initialized()) { return; }

	knifeRestoreOriginals();

	std::vector<Vec2f> path = knifePath;
	if (knifeHasCursor) { path.push_back(knifeCursor); }

	if (path.size() >= 2) {
		KnifeCamera camera;
		//camera.viewProj = renderer.ViewProjection();
		//camera.position = renderer.GetCamera()->position();

		for (auto& original : knifeOriginals) {
			if (original.first >= scene.MeshCount()) { continue; }
			// Cut() leaves the mesh untouched when nothing was hit or the result was not valid
			HalfEdgeMeshKnife knife(*scene[original.first]);
			knife.Cut(camera, path, knifeCutThrough, knifeOccluders);
		}
	}

	makeCurrent();
	//renderer.UpdateRenderBuffers(scene);
}

void Viewport::knifeFinish() {
	// Leaving the tool throws away anything unconfirmed
	knifeHasCursor = false;
	knifeRestoreOriginals();
	knifeSession = false;
	knifePath.clear();
	knifeOriginals.clear();
	knifeOccluders = KnifeOccluders();

	if (renderer.Initialized()) {
		makeCurrent();
		//updateSelectionVisuals();
	}
}

void Viewport::knifeConfirm() {
	if (!knifeSession || knifePath.size() < 2) { return; }

	// The cursor is only a preview. Only clicked points are kept.
	knifeHasCursor = false;
	knifeRecompute();

	// Same tool, next cut starts from the result
	//knifeBegin();

	//makeCurrent();
	//updateSelectionVisuals();
	//update();
}

bool Viewport::knifeCancel() {
	if (!knifeSession || knifePath.empty()) { return false; }

	knifePath.clear();
	knifeRecompute();
	update();
	return true;
}

void Viewport::toggleKnifeCutThrough() {
	knifeCutThrough = !knifeCutThrough;
	knifeRecompute();
	update();
}

void Viewport::drawKnifeOverlay(QPainter& painter) {
	auto toWidget = [this](const Vec2f& ndc) {
		return QPointF((ndc.x * 0.5f + 0.5f) * width(), (1.0f - (ndc.y * 0.5f + 0.5f)) * height());
	};

	painter.setPen(QPen(QColor(255, 220, 60), 2.0));
	for (size_t i = 0; i + 1 < knifePath.size(); ++i) {
		painter.drawLine(toWidget(knifePath[i]), toWidget(knifePath[i + 1]));
	}

	if (knifeHasCursor && !knifePath.empty()) {
		painter.setPen(QPen(QColor(255, 220, 60, 150), 1.0, Qt::DashLine));
		painter.drawLine(toWidget(knifePath.back()), toWidget(knifeCursor));
	}

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(255, 220, 60));
	for (const Vec2f& p : knifePath) { painter.drawEllipse(toWidget(p), 4.0, 4.0); }

	painter.setPen(QColor(255, 255, 255));
	QString hint = "Knife: click to cut, Enter to confirm, Esc to cancel, X cut through: ";
	hint += knifeCutThrough ? "on" : "off";
	if (knifeOriginals.empty()) { hint += "   (select a mesh to cut)"; }
	painter.drawText(QPointF(12.0, 20.0), hint);
}
