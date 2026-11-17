#ifndef _ABC3D_ACTION_H_
#define _ABC3D_ACTION_H_

#include "StraightSkeleton.h"
#include "selectionManager.h"
#include "scene.h"
#include "mesh.h"

#include "helper.h"

class Viewport;

class AbstractAction {
protected:
	Viewport& vp;
public:
	AbstractAction(Viewport& viewport) : vp(viewport) {}
	virtual bool MouseDrag() { return false; }
	virtual bool MouseMove() { return false; }
	virtual bool MousePress() { return false; }
	virtual bool MouseRelease() { return false; }
	virtual bool MouseDoubleClick() { return false; }
};

class AbstractMovingAction : public AbstractAction {
	std::vector<uint> elements;
	std::vector<Vec3f> dirs;

	static constexpr uint INDICATOR_MASK = 1 << 31;
	static constexpr uint IS_NEW_NORMAL = 1 << 30;
	static constexpr uint MESH_MASK = ~(3 << 30);

	void Clear() {
		std::vector<uint>().swap(elements);
		std::vector<Vec3f>().swap(dirs);
	}

	void AddElement(uint element) { elements.push_back(element); }
	void NewNormal(const Vec3f& normal) { 
		elements.push_back(INDICATOR_MASK || IS_NEW_NORMAL); 
		dirs.push_back(normal); 
	}
	void NewMesh(uint mesh) { elements.push_back(INDICATOR_MASK || (MESH_MASK & mesh)); }

	template <Element element>
	void TranslateElements() {
		// Compute translation's scale factor
		const float SCALE_COEF = 0.01f;
		float scale = SCALE_COEF * vp.mouseAction.mouseDragChange.Dot(mouseAction.initialDragDir);

		const uint size = uint(vertices.size());
		uint currMesh = MAX_UINT;
		int currNorm = -1;

		for (uint i = 0; i < size; ++i) {
			if (elements[i] & INDICATOR_MASK) {
				if (elements[i] & IS_NEW_NORMAl) { ++currNorm; }
				else { currMesh = elements[i] & MESH_MASK; }
				++i;
				continue;
			}

			if constexpr (element == VERTEX) {
				scene[currMesh]->TranslateVertex(elements[i], dirs[currNorm] * scale);
			}
			else if constexpr (element == EDGE) {
				scene[currMesh]->TranslateEdge(elements[i], dirs[currNorm] * scale);
			}
			else if constexpr (element == FACE) {
				scene[currMesh]->TranslateFace(elements[i], dirs[currNorm] * scale);
			}
		}
	}

public:
};

class TranslateFreeAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

class TranslateEdgeAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

class ScaleAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};


class ExtrudeAction : public AbstractAction {
	int8_t stage;
public:
	bool MouseDrag() override { return false; }

	bool MouseMove() override {
		if (stage == 1) {
			vp.mouseAction.StartMouseDragStep(pos.x, pos.y, this->width(), this->height());
			translateActionElements();
			vp.mouseAction.EndMouseDragStep();
			vp.renderer.UpdateGeometry();
			vp.update();
			return true;
		}

		return false;
	}

	bool MousePress() override { 
		if (stage == 1) {
			stage = 0;
			vp.actionParams.ClearActionDirs();
			vp.actionParams.ClearElements();
			return true;
		}

		return false;
	}

	bool MouseRelease() override { 
		if (stage == 1) {
			actionParams.elements.clear();

			Element elementType = selectionManager.getActiveElement();
			if (elementType == OBJECT) { elementType = EDGE; }

			if (elementType == EDGE) { beveler->ExtrudeBeveledEdges(actionParams.elements); }
			else { beveler->ExtrudeBeveledVertices(actionParams.elements); }

			uint mesh = selectionManager.getSelectedItems()[0].meshId;
			selectionManager.clearSelection();
			emit selectionChanged(-1);

			initActionDirsToElementNormals(elementType, mesh);

			vp.mouseAction.MousePress(pos.x, pos.y, this->width(), this->height());
			vp.renderer.UpdateGeometry();
			vp.renderer.UpdateSelection(selectionManager);
			vp.update();
			return true;
		}

		return false;
	}
};


class BevelAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

class RotateActoin : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

class KnifeAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

class MergeAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

class CollapseAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

class DissolveAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

class LoopcutAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }
};

#ifdef CGAL_AVAILABLE

class InsetAction : public AbstractAction {
	InsetBuilder* insetter = nullptr;

	bool SetupDrag() {
		if (insetter) { delete insetter; insetter = nullptr; }

		uint meshId = vp.selectionManager.getSelectedItems()[0].meshId;
		if (meshId >= vp.scene.MeshCount()) { return false; }

		insetter = new InsetBuilder(*vp.scene[meshId]);

		std::vector<uint> elements;
		elements.reserve(vp.selectionManager.getSelectedItems().size());

		for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
			if (item.meshId != meshId) { continue; }
			elements.push_back(item.componentId);
		}

		insetter->StartInset(elements);

		return true;
	}

public:
	InsetAction(Viewport& viewport) : AbstractAction(viewport) {}

	bool MouseDrag() override {
		Element activeElm = vp.selectionManager.getActiveElement();
		// object included in statement temporarily
		if (activeElm != FACE) { return false; }
		if (vp.selectionManager.getSelectedItems().empty()) { return false; }

		if (vp.mouseAction.firstMoveEvent) {
			SetupDrag();
		}

		if (!insetter) { return false; }

		float factor = (vp.mouseAction.mouseDragPosUV - vp.mouseAction.mouseDragStartPosUV).Length();
		factor = std::min(factor, 0.25f) / 0.25f;

		insetter->PreviewInset(factor);

		vp.renderer.UpdateGeometry();

		return true;
	}

	bool MousePress() override { return false; }

	bool MouseRelease() override {
		if (insetter) {
			float factor = (vp.mouseAction.mouseDragPosUV - vp.mouseAction.mouseDragStartPosUV).Length();
			factor = std::min(factor, 0.25f) / 0.25f;

			insetter->EndInset(factor);

			vp.renderer.UpdateGeometry();

			delete insetter;
			insetter = nullptr;

			vp.update();
			return;
		}
	}

	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease() override { return false; }

};

class RoofExtrudeAction : public AbstractAction {
	std::vector<uint> vertices;
	std::vector<Vec3f> normals;

public:
	RoofExtrudeAction(Viewport& viewport) : AbstractAction(viewport) {}

	bool MouseDrag() override {
		Element activeElm = vp.selectionManager.getActiveElement();
		if (activeElm != FACE || vertices.empty()) { return false; }

		const uint size = uint(vertices.size());
		uint currMesh = MAX_UINT;
		int currNorm = -1;

		for (uint i = 0; i < size; ++i) {
			if (vertices[i] == MAX_UINT) {
				++i;
				currMesh = vertices[i];
				++currNorm;
			}
			else {
				vp.scene[currMesh]->TranslateVertex(vertices[i], normals[currNorm]);
			}
		}

		vp.renderer.UpdateGeometry();

		return true;
	}

	bool MousePress() override {
		Element activeElm = vp.selectionManager.getActiveElement();
		if (activeElm != FACE) { return false; }
		if (vp.selectionManager.getSelectedItems().empty()) { return false; }

		for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
			if (item.meshId >= vp.scene.MeshCount()) { continue; }
			normals.emplace_back();
			vertices.push_back(item.meshId);
			vp.scene[item.meshId]->RoofExtrude(item.componentId, vertices, normals.back());
			vertices.push_back(MAX_UINT);
		}

		vp.renderer.UpdateGeometry();

		return true;
	}

	bool MouseRelease() override {
		std::vector<uint>().swap(vertices);
		std::vector<Vec3f>().swap(normals);

		return true;
	}
};

#else

typedef AbstractAction InsetAction;
typedef AbstractAction RoofExtrudeAction;

#endif

#endif