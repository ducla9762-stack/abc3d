#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QPushButton>

#include "render.h"
#include "helper.h"
#include "mesh.h"
#include "selectionManager.h"
#include "shapes.h"

inline bool initGlew = true;

class Viewport : public QOpenGLWidget
{
	Q_OBJECT

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
		}

		void StartMouseDragStep(float x, float y, int w, int h) {
			mouseDragPos.Set(x, y);
			mouseDragPosUV = ToUV(mouseDragPos, w, h);
			mouseDragChange = mouseDragPos - mouseDragPrevPos;

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
	bool additiveSelection = true;

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

		renderer.SetCamera(sphericalToCartesian(camPos));
		renderer.UpdateRenderBuffers(scene);
		renderer.UpdateSelection(selectionManager.getSelectedItems());
	}

	void paintGL() override {
		renderer.Draw(defaultFramebufferObject());
	}

	void resizeGL(int w, int h) override {
		renderer.Resize(devicePixelRatio() * w, devicePixelRatio() * h, defaultFramebufferObject());
	}

	// Ported directly from FLTK version
	std::optional<SelectionItem> createSelectionItem(Vec3u ids);

	// Brett's selection logic ported from FLTK Version's Viewport::mousePush
	void handleSelection(const Vec3u& selectionIds) {
		// Create a selection item to send to the selection manager.
		std::optional<SelectionItem> item = createSelectionItem(selectionIds);

		// query the selection maanger with the item.
		selectionManager.processSelection(item, additiveSelection);

		// Tell the renderer to update the selection visuals.
		renderer.UpdateSelection(selectionManager.getSelectedItems());
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

		// Retrieve selection ids from corresponding physical pixel and print them
		Vec3u ids = renderer.SelectionIds(devicePixelRatio() * pos.x, devicePixelRatio() * (pos.y));

		std::cout << "Object Id: " << ids.x << std::endl;
		std::cout << "Component Id: " << ids.y << std::endl;
		std::cout << "Sub-Component Id: " << ids.z << std::endl;

		// Handle selection
		if (ids.x != 0) {
			handleSelection(ids);
		}

		// Update MouseAction struct
		mouseAction.MousePress(pos.x, pos.y, this->width(), this->height());

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
			switch (item.type) {
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

	bool bevelDrag() {
		if (selectionManager.getSelectedItems().empty()) { return false; }

		// Bevel temporarily only works with edges
		if (selectionManager.getActiveElement() != EDGE) { return false; }

		if (mouseAction.firstMoveEvent) {
			if (beveler) { delete beveler; }

			uint meshId = selectionManager.getSelectedItems()[0].meshId;
			if (meshId >= scene.MeshCount()) { return false; }

			std::vector<uint> edges;
			edges.reserve(selectionManager.getSelectedItems().size());

			for (const SelectionItem& item : selectionManager.getSelectedItems()) {
				if (meshId >= scene.MeshCount()) { continue; }
				edges.push_back(item.componentId);
			}

			beveler = new HalfEdgeMeshBeveler(*scene[meshId]);
			beveler->StartEdgeBevel(edges);
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

		return false;
	}

	bool rotateDrag() { return false; }

	void mouseMoveEvent(QMouseEvent* event) override {
		// Accept the event and make the OpenGL context current for renderer
		event->accept();
		makeCurrent();

		// Update MouseAction struct
		mouseAction.StartMouseDragStep(event->position().x(), this->height() - 1.0f - event->position().y(), this->width(), this->height());

		// Perform drag action
		bool actionPerformed = false;

		// Later we'll want to not update render buffers for simple transforms like object level scale, rotate, and translate.
		// We'll do this with a buffer containing objects' model transformations in the shader & renderer
		bool updateRenderBuffers = true;

		switch (actionParams.action) {
		case TRANSLATE_OUT:
			actionPerformed = translateOutDrag();
			break;
		case TRANSLATE_FREE:
			actionPerformed = translateFreeDrag();
			break;
		case TRANSLATE_EDGE:
			actionPerformed = translateEdgeDrag();
			break;
		case SCALE:
			actionPerformed = scaleDrag();
			break;
		case ROTATE:
			actionPerformed = rotateDrag();
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

		// Update MouseAction struct
		mouseAction.EndMouseDragStep();

		update();
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
		camPos.z += float(camChange.y);
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
		renderer.UpdateSelection(selectionManager.getSelectedItems());
		update();
	}

	template<typename T, typename U>
	void loadMesh(const T& vertices, const U& faces) {
		scene.NewMesh()->load(vertices, faces);
		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
	}

public:
	Viewport(QWidget* parent);
	~Viewport();

public slots:
	void setActionExtrude() { actionParams.action = EXTRUDE; }
	void setActionBevel() { actionParams.action = BEVEL; }
	void setActionTranslateFree() { actionParams.action = TRANSLATE_FREE; }
	void setActionTranslateOut() { actionParams.action = TRANSLATE_OUT; }
	void setActionTranslateEdge() { actionParams.action = TRANSLATE_EDGE; }
	void setActionTranslateAxis() { actionParams.action = TRANSLATE_AXIS; }
	void setActionScale() { actionParams.action = SCALE; }
	void setActionRotate() { actionParams.action = ROTATE; }
	void setActionSelect() { actionParams.action = SELECT; }
	void setActionCamera() { actionParams.action = CAMERA; }

	void setElementObject() { selectionManager.setActiveElement(OBJECT); }
	void setElementFace() { selectionManager.setActiveElement(FACE); }
	void setElementEdge() { selectionManager.setActiveElement(EDGE); }
	void setElementVertex() { selectionManager.setActiveElement(VERTEX); }

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
		//renderer.UpdateSelection(selectionManager.getSelectedItems());
		update();
	}
};

