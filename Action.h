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
	Viewport& vw;
public:
	AbstractAction(Viewport& viewport) : vw(viewport) {}
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
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
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

		uint meshId = vw.selectionManager.getSelectedItems()[0].meshId;
		if (meshId >= vw.scene.MeshCount()) { return false; }

		insetter = new InsetBuilder(*vw.scene[meshId]);

		std::vector<uint> elements;
		elements.reserve(vw.selectionManager.getSelectedItems().size());

		for (const SelectionItem& item : vw.selectionManager.getSelectedItems()) {
			if (item.meshId != meshId) { continue; }
			elements.push_back(item.componentId);
		}

		insetter->StartInset(elements);

		return true;
	}

public:
	InsetAction(Viewport& viewport) : AbstractAction(viewport) {}

	bool MouseDrag() override {
		Element activeElm = vw.selectionManager.getActiveElement();
		// object included in statement temporarily
		if (activeElm != FACE) { return false; }
		if (vw.selectionManager.getSelectedItems().empty()) { return false; }

		if (vw.mouseAction.firstMoveEvent) {
			SetupDrag();
		}

		if (!insetter) { return false; }

		float factor = (vw.mouseAction.mouseDragPosUV - vw.mouseAction.mouseDragStartPosUV).Length();
		factor = std::min(factor, 0.25f) / 0.25f;

		insetter->PreviewInset(factor);

		vw.renderer.UpdateGeometry();

		return true;
	}

	bool MousePress() override { return false; }

	bool MouseRelease() override {
		if (insetter) {
			float factor = (vw.mouseAction.mouseDragPosUV - vw.mouseAction.mouseDragStartPosUV).Length();
			factor = std::min(factor, 0.25f) / 0.25f;

			insetter->EndInset(factor);

			vw.renderer.UpdateGeometry();

			delete insetter;
			insetter = nullptr;

			vw.update();
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
		Element activeElm = vw.selectionManager.getActiveElement();
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
				vw.scene[currMesh]->TranslateVertex(vertices[i], normals[currNorm]);
			}
		}

		vw.renderer.UpdateGeometry();

		return true;
	}

	bool MousePress() override {
		Element activeElm = vw.selectionManager.getActiveElement();
		if (activeElm != FACE) { return false; }
		if (vw.selectionManager.getSelectedItems().empty()) { return false; }

		for (const SelectionItem& item : vw.selectionManager.getSelectedItems()) {
			if (item.meshId >= vw.scene.MeshCount()) { continue; }
			normals.emplace_back();
			vertices.push_back(item.meshId);
			vw.scene[item.meshId]->RoofExtrude(item.componentId, vertices, normals.back());
			vertices.push_back(MAX_UINT);
		}

		vw.renderer.UpdateGeometry();

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