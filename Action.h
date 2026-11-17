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
	virtual bool MousePress() { return false; }
	virtual bool MouseRelease() { return false; }
	virtual bool MouseDoubleClick() { return false; }
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
	bool MousePress() override { 
		if (stage == 1) {
			stage = 0;
			vp.actionParams.ClearActionDirs();
			vp.actionParams.ClearElements();
			return;
		}
	}
	bool MouseRelease() override { return false; }
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