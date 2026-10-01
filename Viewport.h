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
#include "MeshBeveler.h"

inline bool initGlew = true;

struct MouseTracker {
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

class Viewport : public QOpenGLWidget
{
	Q_OBJECT

signals:
	void primarySelectionPosChanged(const Vec3f& pos);
	void primarySelectionScaleChanged(const Vec3f& scale);
	void primarySelectionRotChanged(const Vec3f& rot);
	void primarySelectionChanged(bool primarySelection);
	void enableNonVertexSpinners(bool enable);
	void orbitOriginChanged(const Vec3f& origin);

public:

	enum Action {
		CAMERA, SELECT, TRANSLATE_OUT, TRANSLATE_FREE, TRANSLATE_AXIS, TRANSLATE_EDGE,
		SCALE, EXTRUDE, BEVEL, SHAPE, ROTATE
	};

private:

	struct ActionParams {
		Action action = SELECT;
		int stage = 0;
		std::vector<uint> elements;
		std::vector<Vec3f> dirs;
		uint mesh = 0;
		Element elementType = OBJECT; // may be different from selection element type
		void ClearActionDirs() { std::vector<Vec3f>().swap(dirs); }
		void ClearElements() { std::vector<uint>().swap(elements); }
	};

	QAction* menuActions[3];
	QPushButton* elementButtons[4];
	SelectionManager selectionManager;
	ModelingRenderer renderer;
	Scene scene;
	HalfEdgeMeshBeveler* beveler;
	MouseTracker mouseAction;
	ActionParams actionParams;
	bool verticesAlwaysVisible = false;
	bool gridAlwaysVisible = false;
	bool rendererInitialized = false;

	// AI-generated code: Cursor hover state for arrows and active elements.
	uint hoveredGizmoAxis = MAX_UINT;
	Vec3u hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	// AI-generated code: The arrow pressed at the start of an axis drag must
	// remain active even if the cursor moves away from the gizmo.
	uint draggedGizmoAxis = MAX_UINT;
	bool draggingGizmo = false;

// ---------------------------------------------------------------------------------------------
// Core OpenGL ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void initializeGL() override;

	void paintGL() override { renderer.Draw(defaultFramebufferObject()); }

	void resizeGL(int w, int h) override { 
		renderer.Resize(devicePixelRatio() * w, devicePixelRatio() * h, defaultFramebufferObject());
	}

// ---------------------------------------------------------------------------------------------
// Signal Helpers ------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void emitPrimarySelectionPosChanged();
	void emitPrimarySelectionScaleChanged();
	void emitPrimarySelectionRotChanged();
	void emitSelectionChanged();

// ---------------------------------------------------------------------------------------------
// Event Handlers ------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void mousePressEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseDragEvent(const Vec2f& pos); // Not a real qt event
	void leaveEvent(QEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;

// ---------------------------------------------------------------------------------------------
// Drag Actions & Helpers ----------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	bool translateFreeDrag();
	void initActionDirsToSelectedNormals();
	void initActionDirsToElementNormals(Element element, uint mesh);
	void translateSelections();
	void translateActionElements();
	bool translateOutDrag();
	bool translateEdgeDrag() { return false; }
	void setupBevelDrag();
	bool bevelDrag();
	void extrudeSelectedElements();
	bool extrudeDrag();
	bool scaleDrag();
	bool rotateDrag();

// ---------------------------------------------------------------------------------------------
// Selection -----------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void handleSelection(const Vec3u& selectionIds);
	SelectionItem createSelectionItem(const Vec3u& ids);
	void setActiveElement(Element element);

	/// <summary>
	/// Update the selection visuals for the renderer.
	/// </summary>
	void updateSelectionVisuals();

// ---------------------------------------------------------------------------------------------
// Misc. Helpers -------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void setAction(Action action);

	template<typename T, typename U>
	void loadMesh(const T& vertices, const U& faces) {
		scene.NewMesh()->load(vertices, faces);
		makeCurrent();
		renderer.UpdateRenderBuffers(scene);
		update();
	}

	std::vector<uint> exportMeshIds() {
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


// ---------------------------------------------------------------------------------------------
// Hover Interactions --------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	// AI-generated: Reads the ID buffer at the Qt cursor position and forwards
	// the hovered object/component state to both mesh and vertex shaders.
	bool updateHoverVisual(const Vec2f& pos);

// ---------------------------------------------------------------------------------------------
// Gizmos --------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	// Constrain the drag to the world-space axis captured on mouse press.
	bool translateGizmoDrag();

	bool scaleGizmoDrag();

	bool rotateGizmoDrag();

	// Returns the orientation used by the shared gizmo. With multiple selected
	// meshes, the first valid selected mesh supplies the orientation.
	Matrix3f getGizmoRotation() const;

	// AI-generated code: Positions the translation gizmo at the selection average.
	void updateGizmos();

// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

public:
	Viewport(QWidget* parent);
	~Viewport();

public slots:
	void setActionTranslateFree() { setAction(TRANSLATE_FREE); }
	void setActionTranslateOut() { setAction(TRANSLATE_OUT); }
	void setActionTranslateEdge() { setAction(TRANSLATE_EDGE); }
	void setActionTranslateAxis() { setAction(TRANSLATE_AXIS); }

	void setActionExtrude() { setAction(EXTRUDE); }
	void setActionBevel() { setAction(BEVEL); }
	void setActionScale() { setAction(SCALE); }
	void setActionRotate() { setAction(ROTATE); }
	void setActionSelect() { setAction(SELECT); }
	void setActionCamera() { setAction(CAMERA); }


	void setElementVertex() { 
		setActiveElement(VERTEX); 
		elementButtons[0]->setChecked(true);
	}

	void setElementEdge() {
		setActiveElement(EDGE);
		elementButtons[1]->setChecked(true);
	}

	void setElementFace() {
		setActiveElement(FACE);
		elementButtons[2]->setChecked(true);
	}

	void setElementObject() { 
		setActiveElement(OBJECT); 
		elementButtons[3]->setChecked(true);
	}

	void setAdditiveSelection() {
		// If the selection manager is single selection, set it to additive.
		if (!selectionManager.isAdditive()) {
			selectionManager.setAdditive(true);
		}
	}

	void setSingleSelection() {
		// If the selectionManager is set to be additive, and is not in additive mode set to single selection.
		if (selectionManager.isAdditive() && !selectionManager.isAdditiveMode()) {
			selectionManager.setAdditive(false);
		}
	}

	void setClassicSelectionMode() { selectionManager.setAdditiveMode(false); }
	void setAdditiveSelectionMode() { selectionManager.setAdditiveMode(true); }

	void tessellateSelection() {
		if (selectionManager.getSelectedItems().empty()) { return; }

		std::vector<uint> meshes;
		std::unordered_map<uint, std::vector<uint>> meshMap;
		if (selectionManager.getActiveElement() == FACE) {
			meshMap.reserve(selectionManager.meshCount());
		}

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
				if (!meshMap.count(item.meshId)) {
					meshMap[item.meshId].reserve(selectionManager.itemCount(item.meshId));
					meshes.push_back(item.meshId);
				}

				meshMap[item.meshId].push_back(item.componentId);
				break;

			case VERTEX: {
				HalfEdgeMeshBeveler beveler(*scene[item.meshId]);
				beveler.StartVertexBevel({ item.componentId });
				beveler.AdjustBevel(0.5f);
				break;
			}

			default:
				return;
			}
		}

		if (selectionManager.getActiveElement() == FACE) {
			for (uint mesh : meshes) {
				scene[mesh]->tessellateFaces(meshMap[mesh]);
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
			updateGizmos();
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
	void loadIcoSphere() { loadMesh(ICOSPHERE_VERTICES, ICOSPHERE_FACES); }
	void loadUvSphere(){
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

		//camPos = doc.camera;
		//renderer.SetCamera(sphericalToCartesian(camPos));
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
		updateGizmos();
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

	void setAlwaysShowGrid(bool show) {
		gridAlwaysVisible = show;
		renderer.SetGridVisible(gridAlwaysVisible);
		update();
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
		updateGizmos();
		update();
	}

	bool exportObj(const char* filepath) {
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

	bool exportStl(const char* filepath) {
		std::vector<uint> ids = exportMeshIds();
		if (ids.empty()) return true;

		std::vector<Triangle> tris;
		for (uint id : ids)
			scene[id]->appendStlTriangles(tris);

		return writeStl(ensureExtension(filepath, ".stl"), tris, true);
	}

	bool saveScene(const char* filepath) {
		SceneFile doc;

		//doc.camera = camPos; //camTarget is set to ORIGIN and fov fixed at 90.   
		doc.grid = renderer.GridVisible();
		doc.points = renderer.VertexPointsVisible();

		//Object selection id just mesh id
		//sub-object selection is not saved
		if (selectionManager.getActiveElement() == OBJECT) {
			for (const SelectionItem& item : selectionManager.getSelectedItems())
				doc.selection.push_back(item.meshId);
		}

		for (uint id = 0; id < scene.MeshCount(); ++id) {
			const HalfEdgeMesh* mesh = scene[id];
			if (mesh->VertexCount() == 0)
				continue;

			SceneFileObject obj;
			obj.id = id;
			obj.name = "mesh" + std::to_string(id);
			obj.pos = mesh->Pos();
			obj.rot = mesh->Rot();
			obj.scale = mesh->Scale();

			//Local space so we don't apply the transform above twice
			std::ostringstream geometry;
			uint vertOffset = 0;
			mesh->appendObj(geometry, vertOffset, false);
			obj.objText = geometry.str();

			doc.objects.push_back(std::move(obj));
		}

		return WriteSceneFile(filepath, doc);
	}

	void duplicateSelection() {
		if (selectionManager.getActiveElement() != OBJECT) { return; }
		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) { continue; }
			scene.DuplicateMesh(item.meshId);
		}
	}

	void setOrbitOrigin(const Vec3f& origin) {
		renderer.setOrbitOrigin(origin);
		update();
	}

	void orbitSelection() {
		if (selectionManager.itemCount() == 0) { return; }
		Vec3f origin = scene[selectionManager.getSelectedItems()[0].meshId]->Pos();
		setOrbitOrigin(origin);

		emit orbitOriginChanged(origin);
	}

	void setCameraModeOrbit() { renderer.setCameraMode(ORBIT); update(); }

	void setCameraModeFree() { renderer.setCameraMode(FREE); update(); }

};

