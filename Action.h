#ifndef _ABC3D_ACTION_H_
#define _ABC3D_ACTION_H_

#include "StraightSkeleton.h"
#include "selectionManager.h"
#include "MeshBeveler.h"
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
	virtual bool MouseMove(const Vec2f& pos) { return false; }
	virtual bool MousePress() { return false; }
	virtual bool MouseRelease(const Vec2f& pos) { return false; }
	virtual bool MouseDoubleClick() { return false; }
	virtual bool MouseDrag(const Vec2u& selectionIds) { return false; }
};

class AbstractMovingAction : public AbstractAction {
	std::vector<uint> elements;
	std::vector<Vec3f> dirs;

	static constexpr uint INDICATOR_MASK = 1 << 31;
	static constexpr uint IS_NEW_NORMAL = 1 << 30;
	static constexpr uint MESH_MASK = ~(3 << 30);

protected:

	void Clear() {
		std::vector<uint>().swap(elements);
		std::vector<Vec3f>().swap(dirs);
	}

	void NewElement(uint element) { elements.push_back(element); }
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
};

class TranslateFreeAction : public AbstractAction {
public:
	bool MouseDrag() override {
		if (vp.selectionManager.getSelectedItems().empty()) { return false; }

		Vec3f f, u, r;
		vp.renderer.GetCamera()->directions(f, u, r);

		const float SCALE = 0.01f;
		const Vec2f& uv = vp.mouseAction.mouseDragChange;
		Vec3f translation = -SCALE * (uv.x * r + uv.y * u);

		bool transformed = false;

		for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
			if (item.isReferenceImage()) {
				ReferenceImage* image = vp.scene.FindReferenceImage(item.componentId);
				if (image && !image->locked) {
					image->position += translation;
					transformed = true;
				}
				continue;
			}
			if (item.meshId >= vp.scene.MeshCount()) {
				continue;
			}

			// Translate depending on which element type.
			HalfEdgeMesh* mesh = vp.scene[item.meshId];
			switch (vp.selectionManager.getActiveElement()) {
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
			if (vp.selectionManager.getActiveElement() == OBJECT) {
				vp.renderer.UpdateTransforms();
			}
			else {
				vp.renderer.UpdateGeometry();
			}
		}

		return transformed;
	}

	bool MousePress() override { return false; }
	bool MouseRelease(const Vec2f& pos) override { return false; }
};

class TranslateOutAction : public TranslateFreeAction {
	std::vector<Vec3f> normals;
public:
	bool MouseDrag() override {
		// If there are no selected elements, do nothing.
		if (vp.selectionManager.getSelectedItems().empty()) { return false; }

		// If we are selecting objects, translate free instead of out.
		if (vp.selectionManager.getActiveElement() == OBJECT) {
			return TranslateFreeAction::MouseDrag();
		}

		// If this is the first drag step, compute normal directions
		if (vp.mouseAction.firstMoveEvent) {
			Element elm = vp.selectionManager.getActiveElement();
			if (elm == VERTEX) {
				for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
					if (item.meshId >= vp.scene.MeshCount()) { continue; }
					normals.push_back(vp.scene[item.meshId]->GetVertexNormal(item.componentId));
				}
			}
			if (elm == EDGE) {
				for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
					if (item.meshId >= vp.scene.MeshCount()) { continue; }
					normals.push_back(vp.scene[item.meshId]->GetEdgeNormal(item.componentId));
				}
			}
			if (elm == FACE) {
				for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
					if (item.meshId >= vp.scene.MeshCount()) { continue; }
					normals.push_back(vp.scene[item.meshId]->GetFaceNormal(item.componentId));
				}
			}
		}

		if (normals.empty()) { return false; }

		// Translate
		const float SCALE_COEF = 0.01f;
		float scale = SCALE_COEF * vp.mouseAction.mouseDragChange.Dot(vp.mouseAction.initialDragDir);
		Element elm = vp.selectionManager.getActiveElement();

		if (elm == VERTEX) {
			uint element = 0;
			for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
				if (item.meshId >= vp.scene.MeshCount()) { continue; }
				vp.scene[item.meshId]->TranslateVertex(item.componentId, normals[element] * scale);
				++element;
			}
		}
		if (elm == EDGE) {
			uint element = 0;
			for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
				if (item.meshId >= vp.scene.MeshCount()) { continue; }
				vp.scene[item.meshId]->TranslateEdge(item.componentId, normals[element] * scale);
				++element;
			}
		}
		if (elm == FACE) {
			uint element = 0;
			for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
				if (item.meshId >= vp.scene.MeshCount()) { continue; }
				vp.scene[item.meshId]->TranslateFace(item.componentId, normals[element] * scale);
				++element;
			}
		}

		vp.renderer.UpdateGeometry();

		return true;
	}
};

class TranslateEdgeAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease(const Vec2f& pos) override { return false; }
};

class ScaleAction : public AbstractAction {
public:
	bool MouseDrag() override { 
		if (vp.selectionManager.getSelectedItems().empty()) {
			return false;
		}

		float factor = 0.01f * vp.mouseAction.mouseDragChange.Dot(vp.mouseAction.initialDragDir);
		const Element activeElement = vp.selectionManager.getActiveElement();

		if (activeElement == EDGE || activeElement == FACE) {
			const bool scaled = vp.scaleSelectedComponents(factor, 0, true);
			if (scaled) {
				vp.renderer.UpdateGeometry();
			}
			return scaled;
		}

		if (activeElement == VERTEX) {
			return false;
		}

		for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
			if (item.isReferenceImage()) {
				ReferenceImage* image = vp.scene.FindReferenceImage(item.componentId);
				if (image && !image->locked) {
					image->scale += Vec3f(factor);
					image->scale.x = std::max(0.001f, image->scale.x);
					image->scale.y = std::max(0.001f, image->scale.y);
					image->scale.z = std::max(0.001f, image->scale.z);
				}
			}
			else if (item.meshId < vp.scene.MeshCount()) {
				vp.scene[item.meshId]->AddScale(factor);
			}
		}

		vp.renderer.UpdateTransforms();

		return true;
	}

	bool MousePress() override { return false; }
	bool MouseRelease(const Vec2f& pos) override { return false; }
};

class BevelAction : public AbstractAction {
protected:
	HalfEdgeMeshBeveler* beveler;

	void SetupBevelDrag() {
		if (beveler) { delete beveler; }

		uint meshId = vp.selectionManager.getSelectedItems()[0].meshId;
		if (meshId >= vp.scene.MeshCount()) { return; }

		beveler = new HalfEdgeMeshBeveler(*vp.scene[meshId]);

		Element element = vp.selectionManager.getActiveElement();
		if (element == OBJECT) {
			beveler->StartObjectBevel();
			return;
		}

		std::vector<uint> elements;
		elements.reserve(vp.selectionManager.getSelectedItems().size());

		for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
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

		vp.renderer.UpdateSelection(vp.selectionManager);

		return;
	}

public:
	bool MouseDrag() override { 
		if (vp.selectionManager.getSelectedItems().empty()) { return false; }

		if (vp.mouseAction.firstMoveEvent) {
			SetupBevelDrag();
		}

		if (!beveler) { return false; }

		float factor = (vp.mouseAction.mouseDragPosUV - vp.mouseAction.mouseDragStartPosUV).Length();
		factor = std::min(factor, 0.25f) / 0.25f;

		beveler->AdjustBevel(factor);

		vp.renderer.UpdateGeometry();

		return true;
	}

	bool MousePress() override { return false; }
	bool MouseRelease(const Vec2f& pos) override { if (beveler) delete beveler; return false; }
};

class ExtrudeAction : public BevelAction {
	std::vector<uint> elements;
	std::vector<Vec3f> normals;
	uint mesh;
	int8_t stage;
public:
	bool MouseDrag() override {
		if (vp.selectionManager.getSelectedItems().empty()) { return false; }

		if (vp.selectionManager.getActiveElement() != FACE) {
			if (vp.mouseAction.firstMoveEvent) {
				stage = 1;
			}
			return BevelAction::MouseDrag();
		}

		// Extrude selected elements if this is the first move event
		if (vp.mouseAction.firstMoveEvent) {
			for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
				if (item.meshId >= vp.scene.MeshCount()) { continue; }

				vp.scene[item.meshId]->Extrude(item.componentId);
				normals.push_back(vp.scene[item.meshId]->GetFaceNormal(item.componentId));
			}
		}
		else {
			const float SCALE_COEF = 0.01f;
			float scale = SCALE_COEF * vp.mouseAction.mouseDragChange.Dot(vp.mouseAction.initialDragDir);

			uint currNormal = 0;
			for (const SelectionItem& item : vp.selectionManager.getSelectedItems()) {
				if (item.meshId >= vp.scene.MeshCount()) { continue; }

				vp.scene[item.meshId]->TranslateFace(item.componentId, normals[currNormal] * scale);
				++currNormal;
			}
		}

		vp.renderer.UpdateGeometry();
		vp.renderer.UpdateSelection(vp.selectionManager);
		return true;
	}

	bool MouseMove(const Vec2f& pos) override {
		if (stage == 1 && !elements.empty()) {
			const float SCALE_COEF = 0.01f;
			float scale = SCALE_COEF * vp.mouseAction.mouseDragChange.Dot(vp.mouseAction.initialDragDir);

			vp.mouseAction.StartMouseDragStep(pos.x, pos.y, vp.width(), vp.height());
			if (vp.selectionManager.getActiveElement() == VERTEX) {
				for (uint i = 0; i < uint(elements.size(); ++i) {
					vp.scene[mesh]->TranslateVertex(elements[i], normals[i] * scale);
				}
			}
			else {
				for (uint i = 0; i < uint(elements.size(); ++i) {
					vp.scene[mesh]->TranslateEdge(elements[i], normals[i] * scale);
				}
			}

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
			std::vector<uint>().swap(elements);
			std::vector<Vec3f>().swap(normals);
			return true;
		}

		return false;
	}

	bool MouseRelease(const Vec2f& pos) override { 
		if (stage == 1 && beveler) {
			Element elementType = vp.selectionManager.getActiveElement();
			if (elementType == OBJECT) { elementType = EDGE; }

			mesh = vp.selectionManager.getSelectedItems()[0].meshId;
			vp.clearSelection();

			if (elementType == EDGE) {
				beveler->ExtrudeBeveledEdges(elements);
				for (uint element : elements) {
					normals.push_back(vp.scene[mesh]->GetEdgeNormal(element));
				}
			}
			else {
				beveler->ExtrudeBeveledVertices(elements);
				for (uint element : elements) {
					normals.push_back(vp.scene[mesh]->GetVertexNormal(element));
				}
			}

			delete beveler;

			vp.mouseAction.MousePress(pos.x, pos.y, vp.width(), vp.height());
			vp.renderer.UpdateGeometry();
			vp.renderer.UpdateSelection(vp.selectionManager);
			vp.update();
			return true;
		}
		else {
			std::vector<uint>().swap(elements);
			std::vector<Vec3f>().swap(normals);
		}

		return false;
	}
};

class RotateActoin : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease(const Vec2f& pos) override { return false; }
};

class KnifeAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease(const Vec2f& pos) override { return false; }
};

class MergeAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress(const Vec2u& selectionIds) {
		// SelectionIds() returns a zero-based mesh ID and local vertex ID.
		if (selectionIds.x == MAX_UINT || selectionIds.y == MAX_UINT) {
			return;
		}

		const uint meshId = selectionIds.x;
		const uint vertexId = selectionIds.y;
		if (meshId >= vp.scene.MeshCount() || vertexId >= vp.scene[meshId]->VertexCount()) {
			return;
		}

		const SelectionItem clicked{ meshId, vertexId };
		std::cout << "Merge mode vertex clicked: mesh " << meshId
			<< ", vertex " << vertexId << std::endl;

		if (!vp.mergeFirstVertex.has_value()) {
			vp.mergeFirstVertex = clicked;
			vp.selectionManager.clearSelection();
			vp.selectionManager.addSelection(clicked);
			vp.renderer.UpdateSelection(vp.selectionManager);
			vp.renderer.ClearHover();
			vp.emitSelectionChanged();
			vp.update();
			return;
		}

		const SelectionItem source = *vp.mergeFirstVertex;
		if (source == clicked) {
			std::cout << "Merge rejected: choose a different target vertex." << std::endl;
			return;
		}
		if (source.meshId != clicked.meshId) {
			std::cout << "Merge rejected: both vertices must belong to the same mesh." << std::endl;
			return;
		}

		if (!vp.scene[clicked.meshId]->MergeVertices(source.componentId, clicked.componentId)) {
			std::cout << "Merge rejected: the result would have invalid topology." << std::endl;
			return;
		}

		std::cout << "Merged vertex " << source.componentId << " into vertex "
			<< clicked.componentId << " on mesh " << clicked.meshId << std::endl;
		vp.mergeFirstVertex.reset();
		vp.selectionManager.clearSelection();
		vp.renderer.UpdateGeometry();
		vp.renderer.UpdateSelection(vp.selectionManager);
		vp.renderer.ClearHover();
		vp.hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
		vp.emitSelectionChanged();
		vp.update();
	}

	bool MouseRelease() override { return false; }

	bool MouseDoubleClick() override {
		vp.mergeFirstVertex.reset();
	}
};

class CollapseAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress(const Vec2u& selectionIds) {
		// SelectionIds() returns a zero-based mesh ID and local edge ID.
		if (selectionIds.x == MAX_UINT || selectionIds.y == MAX_UINT) {
			return;
		}

		const uint meshId = selectionIds.x;
		const uint edgeId = selectionIds.y;
		if (meshId >= vp.scene.MeshCount() || edgeId >= vp.scene[meshId]->EdgeCount()) {
			return;
		}

		std::cout << "Collapse mode edge clicked: mesh " << meshId
			<< ", edge " << edgeId << std::endl;

		if (!vp.scene[meshId]->CollapseEdge(edgeId)) {
			std::cout << "Edge collapse rejected: the result would have invalid or open topology."
				<< std::endl;
			return;
		}

		std::cout << "Collapsed edge " << edgeId << " on mesh " << meshId << std::endl;
		vp.selectionManager.clearSelection();
		vp.renderer.UpdateGeometry();
		vp.renderer.UpdateSelection(vp.selectionManager);
		vp.renderer.ClearHover();
		vp.hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
		vp.emitSelectionChanged();
		vp.update();
	}

	bool MouseRelease() override { return false; }
};

class DissolveAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress(const Vec2u& selectionIds) {
		// SelectionIds() returns a zero-based mesh ID and local edge ID.
		if (selectionIds.x == MAX_UINT || selectionIds.y == MAX_UINT) {
			return;
		}

		const uint meshId = selectionIds.x;
		const uint edgeId = selectionIds.y;
		if (meshId >= vp.scene.MeshCount() || edgeId >= vp.scene[meshId]->EdgeCount()) {
			return;
		}

		std::cout << "Dissolve mode edge clicked: mesh " << meshId
			<< ", edge " << edgeId << std::endl;

		if (!vp.scene[meshId]->DissolveEdge(edgeId)) {
			std::cout << "Dissolve rejected: choose an interior edge with valid topology."
				<< std::endl;
			return;
		}

		std::cout << "Dissolved edge " << edgeId << " on mesh " << meshId << std::endl;
		vp.selectionManager.clearSelection();
		vp.renderer.UpdateGeometry();
		vp.renderer.UpdateSelection(vp.selectionManager);
		vp.renderer.ClearHover();
		vp.hoveredSelectionIds = Vec3u(MAX_UINT, MAX_UINT, MAX_UINT);
		vp.emitSelectionChanged();
		vp.update();
	}

	bool MouseRelease() override { return false; }
};

class LoopcutAction : public AbstractAction {
public:
	bool MouseDrag() override { return false; }
	bool MousePress() override { return false; }
	bool MouseRelease(const Vec2f& pos) override { return false; }
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

	bool MouseRelease(const Vec2f& pos) override {
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
	bool MouseRelease(const Vec2f& pos) override { return false; }

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

	bool MouseRelease(const Vec2f& pos) override {
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