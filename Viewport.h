#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QPushButton>
#include <QMenu>
#include <QImage>
#include <QPainter>
#include <functional>
#include <optional>
#include <unordered_map>
#include <algorithm>
#include <QtGamepadLegacy/QGamepad>
#include <QtGamepadLegacy/QGamepadManager>

#include "renderer.h"
#include "helper.h"
#include "mesh.h"
#include "selectionManager.h"
#include "shapes.h"
#include "sceneFile.h"
#include "MeshBeveler.h"
#include "MeshKnife.h"
#include "MeshLoopCut.h"
#include "StraightSkeleton.h"

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

// Plain widget that paints on top of the viewport. Drawing with a QPainter inside paintGL depends on the GL state the
// renderer leaves behind, a child widget does not.
class OverlayWidget : public QWidget {
public:
	std::function<void(QPainter&)> draw;
	OverlayWidget(QWidget* parent) : QWidget(parent) {
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_TranslucentBackground);
	}
protected:
	void paintEvent(QPaintEvent*) override {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		if (draw) { draw(painter); }
	}
};

class ViewportMenu;

class Viewport : public QOpenGLWidget
{
	Q_OBJECT

signals:	
	void primarySelectionPosChanged(const Vec3f& pos);
	void primarySelectionScaleChanged(const Vec3f& scale);
	void primarySelectionRotChanged(const Vec3f& rot);
	void primarySelectionChanged(bool primarySelection);
	void enableNonVertexSpinners(bool enable);
	void primaryCamParamChanged(const Vec3f& param);
	void elementChanged(Element element);
	void meshCountChanged(uint meshCount);
	void selectionChanged(int index);

public:

	enum Action {
		CAMERA, SELECT, TRANSLATE_OUT, TRANSLATE_FREE, TRANSLATE_AXIS, TRANSLATE_EDGE,
		SCALE, EXTRUDE, BEVEL, SHAPE, ROTATE, KNIFE, MERGE, COLLAPSE, DISSOLVE, LOOPCUT,
		INSET, ROOF_EXTRUDE
	};

	Element getActiveElement();
	int getMeshCount();
	const std::vector<string>& getNames() const;
	void setName(int index, std::string name);
	void setSelection(int meshID);
	void addSelection(int meshID);

private:

	friend class ViewportMenu;

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

	// OpenGL texture indices are renderer resources, not scene state.
	std::unordered_map<uint, uint> referenceTextureIndices;

	ViewportMenu* menu;
	QPushButton* elementButtons[4];
	SelectionManager selectionManager;
	ABC3DRenderer renderer;
	IconRenderer iconRenderer;
	ImageRenderer imageRenderer;
	Scene scene;
	HalfEdgeMeshBeveler* beveler;
	InsetBuilder* insetter;
	MouseTracker mouseAction;
	ActionParams actionParams;
	bool verticesAlwaysVisible = false;
	bool showPivot = false;
	float pivotScale = 0.05f;
	Vec2f menuPos;

	// AI-generated code: Cursor hover state for arrows and active elements.
	uint hoveredGizmoAxis = MAX_UINT;
	Vec3u hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
	// AI-generated code: The arrow pressed at the start of an axis drag must
	// remain active even if the cursor moves away from the gizmo.
	uint draggedGizmoAxis = MAX_UINT;
	bool draggingGizmo = false;
	// Rotation must keep using the axis that was displayed and clicked at press
	// time, even though the selected mesh may rotate while the drag is active.
	Vec3f draggedRotationAxis = Vec3f(0.0f);

	bool localScale = true;
	std::optional<SelectionItem> mergeFirstVertex;

	// AI-generated code:
	// Edge translation chooses an adjacent edge from the first drag direction.
	// Keep that choice per selected vertex for the duration of the drag so
	// multi-selection does not make every vertex use the same half-edge id.
	std::unordered_map<SelectionItem, uint> edgeDragEdges;
	// Signed movement accumulated along each chosen edge.  This lets the drag
	// clamp at the vertex's original position when the mouse reverses.
	std::unordered_map<SelectionItem, float> edgeDragDistances;

// ---------------------------------------------------------------------------------------------
// Knife ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	// The cut is kept as a path in normalized device coordinates and recomputed from the untouched meshes whenever
	// the path, the cursor or the camera changes. Enter keeps the result, Esc throws it away.
	bool knifeSession = false;
	bool knifeCutThrough = false;
	bool knifeHasCursor = false;
	std::vector<Vec2f> knifePath;
	Vec2f knifeCursor = Vec2f(0.0f);
	std::vector<std::pair<uint, HalfEdgeMesh>> knifeOriginals; // selected meshes as they were before the cut
	KnifeOccluders knifeOccluders; // every mesh in the scene, before the cut

	Vec2f knifeToNdc(const QPointF& pos) const;
	void knifeBegin();
	void knifeRestoreOriginals();
	void knifeRecompute();
	void knifeFinish();
	void drawKnifeOverlay(QPainter& painter);
	OverlayWidget* knifeOverlay = nullptr;

// ---------------------------------------------------------------------------------------------
// Loop Cut ------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	// Hovering an edge previews the cut through its strip of faces. Clicking cuts it.
	uint loopCutCount = 1;
	Vec2u loopCutHover = Vec2u(MAX_UINT, MAX_UINT);           // mesh, half-edge under the cursor
	std::vector<std::pair<Vec3f, Vec3f>> loopCutSegments;     // preview edges, world space
	std::vector<Vec3f> loopCutPoints;                         // preview vertices, world space

	void loopCutUpdatePreview(const Vec2u& selectionIds);
	void loopCutClearPreview();
	void handleLoopCutClick(const Vec2u& selectionIds);
	void drawLoopCutOverlay(QPainter& painter);

// ---------------------------------------------------------------------------------------------
// Core OpenGL ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void initializeGL() override;

	void paintGL() override {
		renderer.Draw(defaultFramebufferObject());
		if (scene.ReferenceImageCount() != 0) {
			float viewProjection[16];
			renderer.GetViewProjection(viewProjection);
			const int pixelWidth = std::max(1, static_cast<int>(width() * devicePixelRatio()));
			const int pixelHeight = std::max(1, static_cast<int>(height() * devicePixelRatio()));
			imageRenderer.SetTransform(viewProjection);
			imageRenderer.BeginPickPass(defaultFramebufferObject(), pixelWidth, pixelHeight);
			for (const ReferenceImage& image : scene.ReferenceImages()) {
				auto texture = referenceTextureIndices.find(image.id);
				if (image.isNull() || !image.visible || texture == referenceTextureIndices.end()) continue;
				imageRenderer.DrawPick(image.position, image.rotation, image.scale, image.id, texture->second);
			}
			imageRenderer.EndPickPass(defaultFramebufferObject(), pixelWidth, pixelHeight);
			for (const ReferenceImage& image : scene.ReferenceImages()) {
				auto texture = referenceTextureIndices.find(image.id);
				if (image.isNull() || !image.visible || texture == referenceTextureIndices.end()) continue;
				imageRenderer.Draw(
					image.position, image.rotation, image.scale, image.opacity,
					selectionManager.hasSelectedReferenceImage(image.id), renderer.SelectionColor(), texture->second,
					defaultFramebufferObject(), pixelWidth, pixelHeight
				);
			}
		}
		renderer.DrawGizmos(defaultFramebufferObject());
		if (showPivot && selectionManager.meshCount() == 1 && selectionManager.getActiveElement() == OBJECT &&
			!selectionManager.getSelectedItems()[0].isReferenceImage()) {
			Vec3f pos = renderer.CanonicalVolumePos(scene[selectionManager.getSelectedItems()[0].meshId]->Pos());
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			iconRenderer.Draw(pos, pivotScale, float(width()) / float(height()));
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
		}
		if (knifeOverlay) { knifeOverlay->setGeometry(rect()); knifeOverlay->raise(); knifeOverlay->setVisible(actionParams.action == KNIFE || actionParams.action == LOOPCUT); knifeOverlay->update(); }
	}

	void resizeGL(int w, int h) override { 
		renderer.Resize(devicePixelRatio() * w, devicePixelRatio() * h, defaultFramebufferObject());
		knifeRecompute();
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
	bool translateEdgeDrag();
	void setupBevelDrag();
	bool bevelDrag();
	void setupInsetDrag();
	bool insetDrag();
	void setupRoofExtrudeDrag();
	bool roofExtrudeDrag();
	void extrudeSelectedElements();
	bool extrudeDrag();
	bool scaleDrag();
	bool scaleSelectedComponents(float amount, uint axis, bool uniform);
	bool rotateDrag();

// ---------------------------------------------------------------------------------------------
// Selection -----------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void handleSelection(const Vec2u& selectionIds);
	void handleSelection(const SelectionItem& item);
	SelectionItem createSelectionItem(const Vec2u& ids);
	void handleMergeVertexClick(const Vec2u& selectionIds);
	void handleCollapseEdgeClick(const Vec2u& selectionIds);
	void handleDissolveEdgeClick(const Vec2u& selectionIds);
	void setActiveElement(Element element);

// ---------------------------------------------------------------------------------------------
// Misc. Helpers -------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void setAction(Action action);

	template<typename T, typename U>
	void loadMesh(const T& vertices, const U& faces, std::string name) {
		scene.NewMesh(name)->load(vertices, faces);
		emit meshCountChanged(scene.MeshCount());
		makeCurrent();
		// 10/16/26 - Update to public API.
		//renderer.UpdateRenderBuffers(scene);
		renderer.UpdateGeometry();
		//renderer.ResetColorBuffer<true>();
		renderer.UpdateSelection(selectionManager);
		//renderer.UpdateUniformBuffer();
		renderer.UpdateTransforms();
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

	Vec2u physicalPixelFromLogical(const Vec2f& logicalPos) const {
		const float dpr = static_cast<float>(devicePixelRatio());
		const int pixelWidth = std::max(1, static_cast<int>(width() * dpr));
		const int pixelHeight = std::max(1, static_cast<int>(height() * dpr));
		const int x = std::clamp(static_cast<int>(std::round(logicalPos.x * dpr)), 0, pixelWidth - 1);
		const int y = std::clamp(static_cast<int>(std::round(logicalPos.y * dpr)), 0, pixelHeight - 1);
		return Vec2u(static_cast<uint>(x), static_cast<uint>(y));
	}

	Vec2u selectionIdsFromLogical(const Vec2f& logicalPos, Element elm) {
		makeCurrent();
		const Vec2u pixel = physicalPixelFromLogical(logicalPos);
		return renderer.SelectionIds(elm, pixel.x, pixel.y);
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
	void setActionTranslateOut() {
		if (getActiveElement() == OBJECT) {
			setActiveElement(FACE);
			elementButtons[2]->setChecked(true);
		}
		setAction(TRANSLATE_OUT); 
	}
	void setActionTranslateAxis() { setAction(TRANSLATE_AXIS); }
	void setActionTranslateEdge() {
		// The constrained operation is vertex only.
		if (selectionManager.getActiveElement() != VERTEX) {
			setActiveElement(VERTEX);
			elementButtons[0]->setChecked(true);
			emit elementChanged(VERTEX);
		}
		// This operation only makes sense on one selection, prevent additive selection with this tool.
		bool reducedSelection = false;
		if (selectionManager.itemCount() > 1) {
			const SelectionItem keep = selectionManager.getSelectedItems().front();
			selectionManager.clearSelection();
			selectionManager.addSelection(keep);
			reducedSelection = true;
		}
		setAction(TRANSLATE_EDGE);
		if (reducedSelection) { emitSelectionChanged(); }
	}

	void setActionExtrude() { setAction(EXTRUDE); }
	void setActionBevel() { setAction(BEVEL); }
	void setActionScale() { 
		if (getActiveElement() == VERTEX) {
			setActiveElement(OBJECT);
			elementButtons[3]->setChecked(true);
			emit selectionChanged(-1);
		}
		setAction(SCALE); }
	void setActionRotate() { 
		if (getActiveElement() != OBJECT) {
			setActiveElement(OBJECT);
			emit selectionChanged(-1);
		}
		elementButtons[3]->setChecked(true);
		setAction(ROTATE); 
	}
	void setActionSelect() { setAction(SELECT); }
	void setActionCamera() { setAction(CAMERA); }
	void setActionKnife() { setAction(KNIFE); }
	void setActionMerge() {
		mergeFirstVertex.reset();
		setActiveElement(VERTEX);
		elementButtons[0]->setChecked(true);
		emit elementChanged(VERTEX);
		setAction(MERGE);
	}

	void setActionCollapse() {
		setActiveElement(EDGE);
		elementButtons[1]->setChecked(true);
		emit elementChanged(EDGE);
		setAction(COLLAPSE);
	}
	void setActionDissolve() {
		setActiveElement(EDGE);
		elementButtons[1]->setChecked(true);
		emit elementChanged(EDGE);
		setAction(DISSOLVE);
	}
	void setActionLoopCut() {
		setActiveElement(EDGE);
		elementButtons[1]->setChecked(true);
		emit elementChanged(EDGE);
		setAction(LOOPCUT);
	}

	void setActionInset() { setAction(INSET); }
	void setActionRoofExtrude() { setAction(ROOF_EXTRUDE); }

	/// <returns>True while the loop cut tool is the active tool.</returns>
	bool loopCutActive() const { return actionParams.action == LOOPCUT; }
	/// <summary>Changes how many parallel cuts the loop cut makes (at least 1).</summary>
	void changeLoopCutCount(int delta);

	/// <returns>True while the knife tool is the active tool.</returns>
	bool knifeActive() const { return actionParams.action == KNIFE; }
	/// <summary>Keeps the cut in progress.</summary>
	void knifeConfirm();
	/// <summary>Throws away the cut in progress.</summary>
	/// <returns>False if there was no cut to throw away.</returns>
	bool knifeCancel();
	/// <summary>Toggles whether the knife also cuts what is hidden from the camera.</summary>
	void toggleKnifeCutThrough();

	void enableAllElements() {
		for (int i = 0; i < 4; ++i) {
			elementButtons[i]->setEnabled(true);
			elementButtons[i]->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
		}
	}

	// Vertex Element
	void setElementVertex() { 
		if (actionParams.action == MERGE || actionParams.action == COLLAPSE ||
			actionParams.action == DISSOLVE || actionParams.action == LOOPCUT) { return; }
		setActiveElement(VERTEX);
		emit elementChanged(VERTEX);
		elementButtons[0]->setChecked(true);
	}

	void enableElementVertex() {
		elementButtons[0]->setEnabled(true);
        elementButtons[0]->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
	}

	void disableElementVertex() {
		elementButtons[0]->setDisabled(true);
		elementButtons[0]->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
	}

	// Edge Element
	void setElementEdge() {
		if (actionParams.action == MERGE) { return; }
		setActiveElement(EDGE);
		emit elementChanged(EDGE);
		elementButtons[1]->setChecked(true);
	}

	void enableElementEdge() {
		elementButtons[1]->setEnabled(true);
		elementButtons[1]->setStyleSheet("QPushButton{ background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
	}

	void disableElementEdge() {
		elementButtons[1]->setDisabled(true);
		elementButtons[1]->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
	}

	// Face Element
	void setElementFace() {
		if (actionParams.action == MERGE || actionParams.action == COLLAPSE ||
			actionParams.action == DISSOLVE || actionParams.action == LOOPCUT) { return; }
		setActiveElement(FACE);
		emit elementChanged(FACE);
		elementButtons[2]->setChecked(true);
	}

	void enableElementFace() {
		elementButtons[2]->setEnabled(true);
		elementButtons[2]->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
	}

	void disableElementFace() {
		elementButtons[2]->setDisabled(true);
		elementButtons[2]->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
	}

	// Object Element
	void setElementObject() { 
		if (actionParams.action == MERGE || actionParams.action == COLLAPSE ||
			actionParams.action == DISSOLVE || actionParams.action == LOOPCUT) { return; }
		setActiveElement(OBJECT); 
		emit elementChanged(OBJECT);
		elementButtons[3]->setChecked(true);
	}

	void enableElementObject() {
		elementButtons[3]->setEnabled(true);
		elementButtons[3]->setStyleSheet("QPushButton { background-color: #646464; color: white; } QPushButton:checked{ background-color: red; }");
	}

	void disableElementObject() {
		elementButtons[3]->setDisabled(true);
		elementButtons[3]->setStyleSheet("QPushButton { background-color: #1c1c1c;}");
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

	void setLocalScaleMode() {
		localScale = true;
		if (renderer.Initialized()) {
			makeCurrent();
			updateGizmos();
		}
		update();
	}
	void setGlobalScaleMode() {
		localScale = false;
		if (renderer.Initialized()) {
			makeCurrent();
			updateGizmos();
		}
		update();
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

		// 10/16/26 - Update to public API.
		renderer.UpdateGeometry();
		renderer.UpdateSelection(selectionManager);

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
		//renderer.UpdateRenderBuffers(scene);
		//renderer.ResetColorBuffer<true>();

		// 10/16/26 - Update to public API
		// Update geometry as topology changes, update selection as well.
		renderer.UpdateGeometry();
		renderer.UpdateSelection(selectionManager);

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
		/*renderer.UpdateRenderBuffers(scene);
		renderer.ResetColorBuffer<true>();*/

		// 10/16/26 - Update to public API
		// Update geometry as topology changes, update selection as well.
		renderer.UpdateGeometry();
		renderer.UpdateSelection(selectionManager);

		update();
	}

	void flipSelection() {
		if (selectionManager.getActiveElement() != EDGE || selectionManager.itemCount() == 0) { return; }

		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) { continue; }

			scene[item.meshId]->flipEdge(item.componentId);
		}

		makeCurrent();
		
		//renderer.UpdateRenderBuffers(scene);

		// 10/16/26 - Update to public API.
		renderer.UpdateGeometry();

		update();
	}

	void setPrimarySelectionPos(const Vec3f& pos) {
		if (selectionManager.meshCount() != 1) { return; }

		const SelectionItem& item = selectionManager.getSelectedItems()[0];

		if (selectionManager.getActiveElement() == VERTEX) {
			if (selectionManager.itemCount() != 1) { return; }
			Vec3f vertPos = scene[item.meshId]->WorldPosToLocalPos(pos);
			scene[item.meshId]->SetVertexPosition(item.componentId, vertPos);

			// 10/16/26 - Updated to public API, in vertex case, update geometry (topology changes).
			renderer.UpdateGeometry();
		}
		else {
			if (item.isReferenceImage()) {
				ReferenceImage* image = scene.FindReferenceImage(item.componentId);
				if (!image || image->locked) { return; }
				image->position = pos;
			}
			else {
				scene[item.meshId]->Pos(pos);
			}

			// In mesh case, update transforms.
			renderer.UpdateTransforms();
		}

		updateGizmos();
		update();
	}

	void setPrimarySelectionScale(const Vec3f& scale) {
		if (selectionManager.meshCount() != 1) { return; }

		if (selectionManager.getActiveElement() != VERTEX) {
			const SelectionItem& item = selectionManager.getSelectedItems()[0];
			if (item.isReferenceImage()) {
				ReferenceImage* image = scene.FindReferenceImage(item.componentId);
				if (!image || image->locked) { return; }
				image->scale = scale;
			}
			else {
				scene[item.meshId]->Scale(scale);
			}

			// 10/16/26 - Updated to public API. Update transforms only.
			renderer.UpdateTransforms();

			updateGizmos();
			update();
		}
	}

	void setPrimarySelectionRot(const Vec3f& rot) {
		if (selectionManager.meshCount() != 1) { return; }

		if (selectionManager.getActiveElement() != VERTEX) {
			const SelectionItem& item = selectionManager.getSelectedItems()[0];
			if (item.isReferenceImage()) {
				ReferenceImage* image = scene.FindReferenceImage(item.componentId);
				if (!image || image->locked) { return; }
				image->rotation = rot;
			}
			else {
				scene[item.meshId]->Rot(rot);
			}

			// 10/16/26 - Updated to public API. Update transforms only.
			renderer.UpdateTransforms();

			updateGizmos();
			update();
		}
	}

	void setPrimaryReferenceImageOpacity(double opacity) {
		if (selectionManager.meshCount() != 1 || selectionManager.getActiveElement() != OBJECT) { return; }
		const SelectionItem& item = selectionManager.getSelectedItems()[0];
		if (!item.isReferenceImage()) { return; }
		ReferenceImage* image = scene.FindReferenceImage(item.componentId);
		if (!image || image->locked) { return; }
		image->opacity = std::clamp(static_cast<float>(opacity), 0.0f, 1.0f);
		update();
	}

	void setPrimaryReferenceImageVisibility(bool visible) {
		if (selectionManager.meshCount() != 1 || selectionManager.getActiveElement() != OBJECT) { return; }
		const SelectionItem& item = selectionManager.getSelectedItems()[0];
		if (!item.isReferenceImage()) { return; }
		ReferenceImage* image = scene.FindReferenceImage(item.componentId);
		if (!image) { return; }
		image->visible = visible;
		update();
	}

	void setPrimaryReferenceImageLocked(bool locked) {
		if (selectionManager.meshCount() != 1 || selectionManager.getActiveElement() != OBJECT) { return; }
		const SelectionItem& item = selectionManager.getSelectedItems()[0];
		if (!item.isReferenceImage()) { return; }
		ReferenceImage* image = scene.FindReferenceImage(item.componentId);
		if (!image) { return; }
		image->locked = locked;
		update();
	}

	void loadCube() {
		loadMesh(CUBE_VERTICES, CUBE_FACES, "Cube");
	}
	void loadPyramid() {
		loadMesh(PYRAMID_VERTICES, PYRAMID_FACES, "Pyramid");
	}
	void loadCylinder(int resolution){
		std::vector<Vec3f> vertices;
		std::vector<Vec4u> faces;
		generateUnitCylinder(vertices, faces, resolution);
		loadMesh(vertices, faces, "Cylinder");
	}
	void loadTorus(int latitude, int longitude){
		std::vector<Vec3f> vertices;
		std::vector<Vec4u> faces;
		generateUnitTorus(vertices, faces, latitude, longitude);
		loadMesh(vertices, faces, "Torus");
	}
	void loadIcoSphere() { 
		loadMesh(ICOSPHERE_VERTICES, ICOSPHERE_FACES, "Ico Sphere");
	}

	void loadUvSphere(int latitude, int longitude){
		std::vector<Vec3f> vertices;
		std::vector<Vec4u> faces;
		generateUVSphere(vertices, faces, latitude, longitude);
		loadMesh(vertices, faces, "UV Sphere");
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
			*scene.NewMesh("Object") = std::move(candidate);
			emit meshCountChanged(scene.MeshCount());
		}

		renderer.UpdateGeometry();
		renderer.UpdateTransforms();
		renderer.UpdateSelection(selectionManager);

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

		//emit meshCountChanged(scene.MeshCount());
		

		scene.Clear();
		emit selectionChanged(-1);
		for (HalfEdgeMesh& candidate : candidates) {
			*scene.NewMesh("Object") = std::move(candidate);
			emit meshCountChanged(scene.MeshCount());
		}

		makeCurrent();
		referenceTextureIndices.clear();
		for (const SceneFileReferenceImage& saved : doc.referenceImages) {
			QImage image(QString::fromStdString(saved.path));
			if (image.isNull()) { continue; }
			image = image.convertToFormat(QImage::Format_RGBA8888);
			image.flip();

			ReferenceImage reference;
			reference.id = saved.id;
			reference.sourcePath = saved.path;
			reference.position = saved.pos;
			reference.rotation = saved.rot;
			reference.scale = saved.scale;
			reference.opacity = saved.opacity;
			reference.visible = saved.visible;
			reference.locked = saved.locked;
			ReferenceImage* added = scene.AddReferenceImage(reference);
			referenceTextureIndices[added->id] = imageRenderer.AddTexture(
				image.constBits(), static_cast<uint>(image.width()),
				static_cast<uint>(image.height()), GL_RGBA);
		}

		//camPos = doc.camera;
		//renderer.SetCamera(sphericalToCartesian(camPos));

		// 10/16/26 - Updated to public API.

		renderer.ShowGrid(doc.grid);
		//renderer.SetVertexPointsVisible(doc.points);

		//setElementObject();
		selectionManager.clearSelection();
		emit selectionChanged(-1);

		for (uint savedId : doc.selection) {
			auto remapped = idMap.find(savedId);
			if (remapped != idMap.end()) {
				selectionManager.selectMesh(remapped->second);
			}
		}
		for (uint imageId : doc.imageSelection) {
			if (scene.FindReferenceImage(imageId) != nullptr) {
				selectionManager.selectReferenceImage(imageId);
			}
		}

		emitSelectionChanged();

		makeCurrent();
		//renderer.SetScene(scene);
		renderer.UpdateGeometry();
		renderer.UpdateTransforms();
		renderer.UpdateSelection(selectionManager);
		//updateSelectionVisuals();
		update();

		return true;
	}

	void setAlwaysShowVertices(bool show) {
		if(selectionManager.getActiveElement() != VERTEX){
			verticesAlwaysVisible = show;
			renderer.ShowVertices(show);
			update();
		}
	}

	void setAlwaysShowGrid(bool show) {
		renderer.ShowGrid(show);
		update();
	}

	// directly ported from FLTK Version's Viewport::deleteElement
	void deleteSelection() {
		if (knifeSession) { return; } // the knife holds mesh ids
		std::cout << "delete" << std::endl;
		std::vector<SelectionItem> selectedItems = selectionManager.getSelectedItems();
		if (selectionManager.getActiveElement() == OBJECT) {
			// Scene::DeleteMesh swaps the last mesh into the removed slot, so
			// delete selected object IDs from highest to lowest.
			std::sort(selectedItems.begin(), selectedItems.end(),
				[](const SelectionItem& left, const SelectionItem& right) {
					return left.meshId > right.meshId;
				});
		}
		else if (selectionManager.getActiveElement() == FACE) {
			// Delete higher face IDs first so removing a face does not shift the
			// IDs of selected faces that are still waiting to be deleted.
			std::sort(selectedItems.begin(), selectedItems.end(),
				[](const SelectionItem& left, const SelectionItem& right) {
					if (left.meshId != right.meshId) {
						return left.meshId < right.meshId;
					}
					return left.componentId > right.componentId;
				});
		}

		for (const SelectionItem& item : selectedItems) {
			if (item.isReferenceImage()) {
				referenceTextureIndices.erase(item.componentId);
				scene.DeleteReferenceImage(item.componentId);
				continue;
			}
			if (item.meshId >= scene.MeshCount()) {
				continue;
			}

			switch (selectionManager.getActiveElement()) {
			case OBJECT:
				scene.DeleteMesh(item.meshId);
				emit meshCountChanged(scene.MeshCount());
				break;

			case VERTEX:
				// todo
				break;

			case EDGE:
				// Dissolve edge as delete operation.
				scene[item.meshId]->DissolveEdge(item.componentId);
				break;

			case FACE:
				// Delete faces in descending ID order; see the sort above.
				scene[item.meshId]->DeleteFace(item.componentId);
				break;
			}
		}

		makeCurrent();
		selectionManager.clearSelection();
		hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
		hoveredGizmoAxis = MAX_UINT;
		draggingGizmo = false;
		draggedGizmoAxis = MAX_UINT;
		draggedRotationAxis = Vec3f(0.0f);
		renderer.ClearHover();
		updateGizmos();
		//renderer.UpdateRenderBuffers(scene);
		//renderer.ResetColorBuffer<true>();
		//renderer.UpdateUniformBuffer();
		//updateSelectionVisuals();

		// 10/16/26 - Changed to public API.
		// Need to update geometry because the mesh list/IDs/vertex buffers have changed.
		// Need to update mesh transforms (?)
		// Clear old highlights

		renderer.UpdateGeometry();
		renderer.UpdateTransforms();
		renderer.UpdateSelection(selectionManager);

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

		// 10/16/26 - Updated to new public API
		doc.grid = renderer.ShowGrid();
		doc.points = renderer.ShowVertices();

		//Object selection id just mesh id
		//sub-object selection is not saved
		if (selectionManager.getActiveElement() == OBJECT) {
			for (const SelectionItem& item : selectionManager.getSelectedItems()) {
				if (item.isReferenceImage()) doc.imageSelection.push_back(item.componentId);
				else doc.selection.push_back(item.meshId);
			}
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

		for (const ReferenceImage& image : scene.ReferenceImages()) {
			SceneFileReferenceImage saved;
			saved.id = image.id;
			saved.path = image.sourcePath;
			saved.pos = image.position;
			saved.rot = image.rotation;
			saved.scale = image.scale;
			saved.opacity = image.opacity;
			saved.visible = image.visible;
			saved.locked = image.locked;
			doc.referenceImages.push_back(std::move(saved));
		}

		return WriteSceneFile(filepath, doc);
	}

	void duplicateSelection() {
		if (knifeSession) { return; }
		if (selectionManager.getActiveElement() != OBJECT) { return; }
		for (const SelectionItem& item : selectionManager.getSelectedItems()) {
			if (item.meshId >= scene.MeshCount()) { continue; }
			scene.DuplicateMesh(item.meshId, scene.Names()[item.meshId]);
			emit meshCountChanged(scene.MeshCount());

			renderer.UpdateGeometry();
			renderer.UpdateSelection(selectionManager);
			renderer.UpdateTransforms();

			update();
		}
	}

	void setOrbitOrigin(const Vec3f& origin) {
		renderer.setOrbitOrigin(origin);
		emit primaryCamParamChanged(origin);
		update();
	}

	void setPrimaryCamParam(const Vec3f& param) {
		renderer.setPrimaryCamParam(param);
		update();
	}

	void orbitSelection() {
		if (selectionManager.itemCount() == 0) { return; }
		Vec3f origin = scene[selectionManager.getSelectedItems()[0].meshId]->Pos();
		setOrbitOrigin(origin);

		emit primaryCamParamChanged(origin);
	}

	void setCameraModeOrbit() {
		//std::cout << renderer.getCameraPosition() << std::endl;
		renderer.setCameraMode(ORBIT); knifeRecompute(); update(); 
		emit primaryCamParamChanged(renderer.getPrimaryCamParam());
	}

	void setCameraModeFree() { 
		//std::cout << renderer.getCameraPosition() << std::endl;
		renderer.setCameraMode(FREE); knifeRecompute(); update(); 
		emit primaryCamParamChanged(renderer.getPrimaryCamParam());
	}

	void SetFOV(double fov) {
		if (renderer.Initialized()) {
			makeCurrent();
			renderer.FOV<true>(fov);
			update();
		}
		else {
			renderer.FOV<false>(fov);
		}
	}

	void SetSurfaceVisibility(bool show) { renderer.ShowSurfaces(show); update(); }

	void SetEdgeVisibility(bool show) { renderer.ShowEdges(show); update(); }

	void SetFaceColor(const Vec3f& clr) { 
		if (renderer.Initialized()) {
			makeCurrent(); 
			renderer.FaceColor<true>(clr); 
			update();
		}
		else {
			renderer.FaceColor<false>(clr);
		}
	}

	void SetEdgeColor(const Vec3f& clr) { 
		if (renderer.Initialized()) {
			makeCurrent();
			renderer.EdgeColor<true>(clr);
			update();
		}
		else {
			renderer.EdgeColor<false>(clr);
		}
	}

	void SetVertexColor(const Vec3f& clr) { 
		if (renderer.Initialized()) {
			makeCurrent();
			renderer.VertexColor<true>(clr);
			update();
		}
		else {
			renderer.VertexColor<false>(clr);
		}
	}

	void SetBackgroundColor(const Vec3f& clr) { 
		if (renderer.Initialized()) {
			makeCurrent();
			renderer.BackgroundColor<true>(clr);
			update();
		}
		else {
			renderer.BackgroundColor<false>(clr);
		}
	}

	void SetSelectColor(const Vec3f& clr) {
		if (renderer.Initialized()) {
			makeCurrent();
			renderer.SelectColor<true>(clr);
			update();
		}
		else {
			renderer.SelectColor<false>(clr);
		}
	}

	void SetHoverColor(const Vec3f& clr) {
		if (renderer.Initialized()) {
			makeCurrent();
			renderer.HoverColor<true>(clr);
			update();
		}
		else {
			renderer.HoverColor<false>(clr);
		}
	}

	void SetEdgeRadius(double radius) { makeCurrent(); renderer.SetEdgeRadius(radius); update(); }

	void SetVertexScale(double scale){ makeCurrent(); renderer.SetVertexScale(scale); update(); }

	void setPivotVisibility(bool visible) { if (showPivot != visible) { showPivot = visible; update(); } }

	void setPivotScale(double scale) { pivotScale = float(scale); update(); }

	void loadReferenceImage(const QString& filepath) {
		QImage image(filepath);
		if (image.isNull()) { return; }

		image = image.convertToFormat(QImage::Format_RGBA8888);
		image.flip();

		makeCurrent();

		ReferenceImage* reference = scene.NewReferenceImage(filepath.toStdString());
		reference->position = Vec3f(0.0f);
		referenceTextureIndices[reference->id] = imageRenderer.AddTexture(
			image.constBits(),
			static_cast<uint>(image.width()),
			static_cast<uint>(image.height()),
			GL_RGBA
		);

		update();
	}

private slots:
	void rightClickDuplicate() {
		Vec2u ids = selectionIdsFromLogical(menuPos, OBJECT);
		qDebug() << "pickedMeshiD =" << ids.x << " scene Mesh Count = " << scene.MeshCount();
		if (ids.x != MAX_UINT && ids.x < scene.MeshCount()) {
			scene.DuplicateMesh(ids.x, scene.Names()[ids.x]);
			emit meshCountChanged(scene.MeshCount());

			renderer.UpdateGeometry();
			renderer.UpdateSelection(selectionManager);
			renderer.UpdateTransforms();
			update();
		}
	}

	void rightClickDelete() {
		Vec2u ids = selectionIdsFromLogical(menuPos, OBJECT);
		if (ids.x != MAX_UINT && ids.x < scene.MeshCount()) {
			deleteSelection();
			emit meshCountChanged(scene.MeshCount());
			emit selectionChanged(-1);

			renderer.UpdateGeometry();
			renderer.UpdateSelection(selectionManager);
			renderer.UpdateTransforms();
			update();
		}
	}

	void rightClickOrbit() {
		Vec2u ids = selectionIdsFromLogical(menuPos, OBJECT);
		if (ids.x != MAX_UINT && ids.x < scene.MeshCount()) {
			renderer.setOrbitOrigin(scene[ids.x]->Pos());
			update();
			emit primaryCamParamChanged(renderer.getPrimaryCamParam());
		}
	}

	void rightClickRecenter() {
		Vec2u ids = selectionIdsFromLogical(menuPos, OBJECT);
		if (ids.x != MAX_UINT && ids.x < scene.MeshCount()) {
			scene[ids.x]->RecenterOnCentroid();

			renderer.UpdateGeometry();
			renderer.UpdateSelection(selectionManager);
			renderer.UpdateTransforms();

			update();

			if (selectionManager.hasSelectedMesh(ids.x)) {
				emitPrimarySelectionPosChanged();
			}
		}
	}

	void rightClickBakeScale() {
		Vec2u ids = selectionIdsFromLogical(menuPos, OBJECT);
		if (ids.x != MAX_UINT && ids.x < scene.MeshCount()) {
			scene[ids.x]->BakeScale();

			renderer.UpdateGeometry();
			renderer.UpdateSelection(selectionManager);
			renderer.UpdateTransforms();

			if (selectionManager.hasSelectedMesh(ids.x)) {
				emitPrimarySelectionScaleChanged();
			}
		}
	}

	void rightClickCenterOnBoundary(Axis boundary) {
		Vec2u ids = selectionIdsFromLogical(menuPos, OBJECT);
		if (ids.x != MAX_UINT && ids.x < scene.MeshCount()) {
			scene[ids.x]->RecenterOnBoundary(boundary);

			renderer.UpdateGeometry();
			renderer.UpdateSelection(selectionManager);
			renderer.UpdateTransforms();

			update();

			if (selectionManager.hasSelectedMesh(ids.x)) {
				emitPrimarySelectionPosChanged();
			}
		}
	}

	void rightClickCenterOnCursor() {
		Vec2u pixel = physicalPixelFromLogical(menuPos);
		Vec2u ids; Vec3f pos;
		bool hit = renderer.QueryFace(pixel.x, pixel.y, &ids, &pos);
		if (!hit || ids.x >= scene.MeshCount()) { return; }

		scene[ids.x]->RecenterOn(scene[ids.x]->WorldPosToLocalPos(pos));

		renderer.UpdateGeometry();
		renderer.UpdateSelection(selectionManager);
		renderer.UpdateTransforms();

		update();

		if (selectionManager.hasSelectedMesh(ids.x)) {
			emitPrimarySelectionPosChanged();
		}
	}

	void rightClickCenterOnFace() {
		Vec2u pixel = physicalPixelFromLogical(menuPos);
		Vec2u ids;
		bool hit = renderer.QueryFace(pixel.x, pixel.y, &ids);
		if (!hit || ids.x >= scene.MeshCount()) { return; }

		scene[ids.x]->RecenterOn(scene[ids.x]->faceCentroid(ids.y));

		renderer.UpdateGeometry();
		renderer.UpdateSelection(selectionManager);
		renderer.UpdateTransforms();

		update();

		if (selectionManager.hasSelectedMesh(ids.x)) {
			emitPrimarySelectionPosChanged();
		}
	}

	void rightClickCenterOnBoundaryX() { rightClickCenterOnBoundary(X); }
	void rightClickCenterOnBoundaryXNeg() { rightClickCenterOnBoundary(X_NEG); }
	void rightClickCenterOnBoundaryY() { rightClickCenterOnBoundary(Y); }
	void rightClickCenterOnBoundaryYNeg() { rightClickCenterOnBoundary(Y_NEG); }
	void rightClickCenterOnBoundaryZ() { rightClickCenterOnBoundary(Z); }
	void rightClickCenterOnBoundaryZNeg() { rightClickCenterOnBoundary(Z_NEG); }
};

class ViewportMenu : public QMenu {
	//QAction* actions[4];
	//QMenu* menus[1];
	friend class Viewport;

public:
	ViewportMenu(Viewport* viewport) : QMenu(viewport)
	{
		/*actions[0] = new QAction("Delete", viewport);
		actions[1] = new QAction("Duplicate", viewport);
		actions[2] = new QAction("Orbit Camera", viewport);
		actions[3] = new QAction("Bake Scale", viewport);

		QObject::connect(actions[0], &QAction::triggered, this, &Viewport::rightClickDelete);
		QObject::connect(actions[1], &QAction::triggered, this, &Viewport::rightClickDuplicate);
		QObject::connect(actions[2], &QAction::triggered, this, &Viewport::rightClickOrbit);
		QObject::connect(actions[3], &QAction::triggered, this, &Viewport::rightClickRecenter);
		QObject::connect(menuActions[4], &QAction::triggered, this, &Viewport::rightClickBakeScale);*/
		QAction* action = QMenu::addAction("Delete");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickDelete);

		action = QMenu::addAction("Duplicate");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickDuplicate);

		action = QMenu::addAction("Orbit Camera");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickOrbit);

		action = QMenu::addAction("Bake Scale");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickBakeScale);

		QMenu* menu = QMenu::addMenu("Set Pivot");

		action = menu->addAction("Center");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickRecenter);

		QMenu* facePivMenu = menu->addMenu("Face (if applicable)");

		action = facePivMenu->addAction("Center");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickCenterOnFace);

		action = facePivMenu->addAction("Cursor");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickCenterOnCursor);

		QMenu* boundaryMenu = menu->addMenu("Boundary");

		action = boundaryMenu->addAction("X");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickCenterOnBoundaryX);

		action = boundaryMenu->addAction("-X");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickCenterOnBoundaryXNeg);

		action = boundaryMenu->addAction("Y");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickCenterOnBoundaryY);

		action = boundaryMenu->addAction("-Y");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickCenterOnBoundaryYNeg);

		action = boundaryMenu->addAction("Z");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickCenterOnBoundaryZ);

		action = boundaryMenu->addAction("-Z");
		QObject::connect(action, &QAction::triggered, viewport, &Viewport::rightClickCenterOnBoundaryZNeg);
	}

};
