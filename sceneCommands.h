#ifndef _ABC3D_SCENE_COMMANDS_H_
#define _ABC3D_SCENE_COMMANDS_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "undoHistory.h"

// Commands that change the document by swapping stored state with live state. A command is
// built after its edit already happened and stores the state from before the edit. Undo swaps
// it in (the command now holds the edited state), redo swaps it back.

inline bool SameReferenceImage(const ReferenceImage& a, const ReferenceImage& b) {
	return a.id == b.id && a.sourcePath == b.sourcePath &&
		a.position == b.position && a.rotation == b.rotation && a.scale == b.scale &&
		a.opacity == b.opacity && a.visible == b.visible && a.locked == b.locked;
}

inline bool SameReferenceImages(const std::vector<ReferenceImage>& a, const std::vector<ReferenceImage>& b) {
	if (a.size() != b.size()) { return false; }
	for (size_t i = 0; i < a.size(); ++i) {
		if (!SameReferenceImage(a[i], b[i])) { return false; }
	}
	return true;
}

/// <summary>Object-level transform of one mesh.</summary>
struct MeshTransform {
	Vec3f pos = Vec3f(0.0f);
	Vec3f rot = Vec3f(0.0f);
	Vec3f scale = Vec3f(1.0f);

	static MeshTransform Of(const HalfEdgeMesh& mesh) { return { mesh.Pos(), mesh.Rot(), mesh.Scale() }; }
	void ApplyTo(HalfEdgeMesh& mesh) const { mesh.Pos(pos); mesh.Rot(rot); mesh.Scale(scale); }
	bool operator==(const MeshTransform& o) const { return pos == o.pos && rot == o.rot && scale == o.scale; }
	bool operator!=(const MeshTransform& o) const { return !(*this == o); }
};

// ---------------------------------------------------------------------------------------------
// Swap commands -------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

/// <summary>Base for commands whose undo and redo are the same swap.</summary>
struct SwapCommand : Command {
	virtual void swapState(DocumentRefs& doc) = 0;
	void undo(DocumentRefs& doc) override { swapState(doc); }
	void redo(DocumentRefs& doc) override { swapState(doc); }
};

/// <summary>Element mode plus selected items.</summary>
class SelectionCommand : public SwapCommand {
	SelectionState stored;
public:
	explicit SelectionCommand(SelectionState before) : stored(std::move(before)) {}
	void swapState(DocumentRefs& doc) override {
		SelectionState live = doc.selection.getState();
		doc.selection.setState(stored);
		stored = std::move(live);
	}
	size_t byteSize() const override { return sizeof(*this) + stored.items.capacity() * sizeof(SelectionItem); }
	const char* label() const override { return "Select"; }
};

/// <summary>
/// The whole reference image list. Images are small structs (the texture is a renderer resource
/// reloaded from sourcePath), so one list covers add, delete, transforms, and property changes.
/// </summary>
class ReferenceImagesCommand : public SwapCommand {
	std::vector<ReferenceImage> stored;
public:
	explicit ReferenceImagesCommand(std::vector<ReferenceImage> before) : stored(std::move(before)) {}
	void swapState(DocumentRefs& doc) override { std::swap(doc.scene.ReferenceImages(), stored); }
	size_t byteSize() const override {
		size_t total = sizeof(*this) + stored.capacity() * sizeof(ReferenceImage);
		for (const ReferenceImage& image : stored) { total += image.sourcePath.capacity(); }
		return total;
	}
	const char* label() const override { return "Reference Image"; }
};

class DisplaySettingsCommand : public SwapCommand {
	DisplaySettings stored;
public:
	explicit DisplaySettingsCommand(const DisplaySettings& before) : stored(before) {}
	void swapState(DocumentRefs& doc) override { std::swap(doc.display, stored); }
	size_t byteSize() const override { return sizeof(*this); }
	const char* label() const override { return "Display Setting"; }
};

/// <summary>Object transforms of the meshes at the stored indices.</summary>
class TransformCommand : public SwapCommand {
	std::vector<std::pair<uint, MeshTransform>> stored;
public:
	void add(uint index, const MeshTransform& before) { stored.emplace_back(index, before); }
	bool empty() const { return stored.empty(); }
	void swapState(DocumentRefs& doc) override {
		for (auto& [index, transform] : stored) {
			if (index >= doc.scene.MeshCount()) { continue; }
			HalfEdgeMesh& mesh = *doc.scene[index];
			MeshTransform live = MeshTransform::Of(mesh);
			transform.ApplyTo(mesh);
			transform = live;
		}
	}
	size_t byteSize() const override { return sizeof(*this) + stored.capacity() * sizeof(stored[0]); }
	const char* label() const override { return "Transform"; }
};

/// <summary>
/// Full copies of the meshes at the stored indices. Used for anything that can change topology,
/// since element ids renumber and the topology operators have no inverse.
/// </summary>
class MeshContentCommand : public SwapCommand {
	std::vector<std::pair<uint, HalfEdgeMesh>> stored;
public:
	void add(uint index, HalfEdgeMesh&& before) { stored.emplace_back(index, std::move(before)); }
	bool empty() const { return stored.empty(); }
	void swapState(DocumentRefs& doc) override {
		for (auto& [index, mesh] : stored) {
			if (index >= doc.scene.MeshCount()) { continue; }
			std::swap(*doc.scene[index], mesh);
		}
	}
	size_t byteSize() const override {
		size_t total = sizeof(*this) + stored.capacity() * sizeof(stored[0]);
		for (const auto& entry : stored) { total += entry.second.MemoryBytes() - sizeof(HalfEdgeMesh); }
		return total;
	}
	const char* label() const override { return "Edit Mesh"; }
};

/// <summary>
/// Vertex positions only, for edits that move vertices without changing topology.
/// Much smaller than a full mesh copy.
/// </summary>
class VertexPositionsCommand : public SwapCommand {
	std::vector<std::pair<uint, std::vector<Vec3f>>> stored;
public:
	void add(uint index, std::vector<Vec3f>&& before) { stored.emplace_back(index, std::move(before)); }
	bool empty() const { return stored.empty(); }
	void swapState(DocumentRefs& doc) override {
		for (auto& [index, positions] : stored) {
			if (index >= doc.scene.MeshCount()) { continue; }
			doc.scene[index]->SwapVertexPositions(positions);
		}
	}
	size_t byteSize() const override {
		size_t total = sizeof(*this) + stored.capacity() * sizeof(stored[0]);
		for (const auto& entry : stored) { total += entry.second.capacity() * sizeof(Vec3f); }
		return total;
	}
	const char* label() const override { return "Move Vertices"; }
};

// ---------------------------------------------------------------------------------------------
// Structural commands -------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

/// <summary>
/// A mesh that was appended at index. Holds nothing while the mesh is in the scene; undo takes
/// the mesh (and its name) out of the scene and keeps it for redo.
/// </summary>
class AddMeshCommand : public Command {
	uint index;
	std::unique_ptr<HalfEdgeMesh> held;
	std::string name;
public:
	explicit AddMeshCommand(uint index) : index(index) {}
	void undo(DocumentRefs& doc) override {
		if (index >= doc.scene.MeshCount()) { return; }
		auto detached = doc.scene.DetachMesh(index);
		held = std::move(detached.first);
		name = std::move(detached.second);
	}
	void redo(DocumentRefs& doc) override {
		if (!held) { return; }
		doc.scene.InsertMeshAt(index, std::move(held), std::move(name));
	}
	size_t byteSize() const override { return sizeof(*this) + (held ? held->MemoryBytes() : 0) + name.capacity(); }
	const char* label() const override { return "Add Object"; }
};

/// <summary>
/// A mesh that was removed with Scene::DetachMesh(index). Constructed holding the removed mesh;
/// undo puts it back with the exact inverse of the swap-remove.
/// </summary>
class DeleteMeshCommand : public Command {
	uint index;
	std::unique_ptr<HalfEdgeMesh> held;
	std::string name;
public:
	DeleteMeshCommand(uint index, std::unique_ptr<HalfEdgeMesh> mesh, std::string name)
		: index(index), held(std::move(mesh)), name(std::move(name)) {}
	void undo(DocumentRefs& doc) override {
		if (!held) { return; }
		doc.scene.InsertMeshAt(index, std::move(held), std::move(name));
	}
	void redo(DocumentRefs& doc) override {
		if (index >= doc.scene.MeshCount()) { return; }
		auto detached = doc.scene.DetachMesh(index);
		held = std::move(detached.first);
		name = std::move(detached.second);
	}
	size_t byteSize() const override { return sizeof(*this) + (held ? held->MemoryBytes() : 0) + name.capacity(); }
	const char* label() const override { return "Delete Object"; }
};

// ---------------------------------------------------------------------------------------------
// Transaction ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

/// <summary>
/// Records one undo step around an edit that happens gradually (a drag) or touches several kinds
/// of state. begin() captures the cheap state (selection, reference images, display settings,
/// every mesh's transform). Before changing a mesh, the caller "touches" it so its before-state
/// is captured once: touchMesh for topology edits, touchVertexPositions for vertex moves.
/// Adding or removing meshes is recorded as it happens with recordStructural. commit() compares
/// everything with the live document and returns a command holding only what changed, or null.
///
/// Meshes are tracked by pointer, which Scene keeps stable, so the before-state of a mesh is
/// still matched to it when another mesh is removed during the same step.
/// </summary>
class DocumentTransaction {
	bool active = false;
	std::string name;

	std::vector<HalfEdgeMesh*> meshesAtBegin;
	std::unordered_map<const HalfEdgeMesh*, uint> beginIndexOf;
	std::vector<MeshTransform> transformsAtBegin;
	std::map<uint, HalfEdgeMesh> meshesBefore;           // begin index -> full copy
	std::map<uint, std::vector<Vec3f>> positionsBefore;  // begin index -> vertex positions
	SelectionState selectionBefore;
	std::vector<ReferenceImage> imagesBefore;
	DisplaySettings displayBefore;
	std::vector<std::unique_ptr<Command>> structural;    // in the order they happened

	bool beginIndex(DocumentRefs& doc, uint currentIndex, uint& index) const {
		if (!active || currentIndex >= doc.scene.MeshCount()) { return false; }
		auto found = beginIndexOf.find(doc.scene[currentIndex]);
		if (found == beginIndexOf.end()) { return false; } // added during this step
		index = found->second;
		return true;
	}

public:
	bool isActive() const { return active; }
	const std::string& label() const { return name; }
	void setLabel(std::string label) { name = std::move(label); }

	void reset() {
		active = false;
		name.clear();
		meshesAtBegin.clear();
		beginIndexOf.clear();
		transformsAtBegin.clear();
		meshesBefore.clear();
		positionsBefore.clear();
		selectionBefore = SelectionState();
		imagesBefore.clear();
		structural.clear();
	}

	void begin(DocumentRefs& doc, std::string label) {
		reset();
		active = true;
		name = std::move(label);
		const uint count = doc.scene.MeshCount();
		meshesAtBegin.reserve(count);
		transformsAtBegin.reserve(count);
		for (uint i = 0; i < count; ++i) {
			HalfEdgeMesh* mesh = doc.scene[i];
			meshesAtBegin.push_back(mesh);
			beginIndexOf[mesh] = i;
			transformsAtBegin.push_back(MeshTransform::Of(*mesh));
		}
		selectionBefore = doc.selection.getState();
		imagesBefore = doc.scene.ReferenceImages();
		displayBefore = doc.display;
	}

	/// <summary>Captures a full copy of the mesh at currentIndex, once per step.</summary>
	void touchMesh(DocumentRefs& doc, uint currentIndex) {
		uint index;
		if (!beginIndex(doc, currentIndex, index) || meshesBefore.count(index)) { return; }
		meshesBefore.emplace(index, *doc.scene[currentIndex]);
	}

	/// <summary>Same as touchMesh, with a before-copy the caller already made (the knife's).</summary>
	void touchMesh(DocumentRefs& doc, uint currentIndex, HalfEdgeMesh&& before) {
		uint index;
		if (!beginIndex(doc, currentIndex, index) || meshesBefore.count(index)) { return; }
		meshesBefore.emplace(index, std::move(before));
	}

	/// <summary>Captures the vertex positions of the mesh at currentIndex, once per step.</summary>
	void touchVertexPositions(DocumentRefs& doc, uint currentIndex) {
		uint index;
		if (!beginIndex(doc, currentIndex, index) || meshesBefore.count(index) || positionsBefore.count(index)) { return; }
		positionsBefore.emplace(index, doc.scene[currentIndex]->VertexPositionList());
	}

	/// <summary>Adds a mesh add/delete that already happened.</summary>
	void recordStructural(std::unique_ptr<Command> cmd) {
		if (active && cmd) { structural.push_back(std::move(cmd)); }
	}

	/// <returns>The step, or null when nothing changed. The transaction is reset either way.</returns>
	std::unique_ptr<Command> commit(DocumentRefs& doc) {
		if (!active) { return nullptr; }

		// Per-mesh changes are stored by begin index. They come first in the composite, so undo
		// applies them last, after the structural commands have restored the begin mesh order.
		auto transforms = std::make_unique<TransformCommand>();
		for (uint i = 0; i < meshesAtBegin.size(); ++i) {
			if (meshesBefore.count(i)) { continue; } // the full copy includes the transform
			if (MeshTransform::Of(*meshesAtBegin[i]) != transformsAtBegin[i]) {
				transforms->add(i, transformsAtBegin[i]);
			}
		}

		auto meshes = std::make_unique<MeshContentCommand>();
		for (auto& [index, before] : meshesBefore) {
			if (!meshesAtBegin[index]->SameState(before)) { meshes->add(index, std::move(before)); }
		}

		auto positions = std::make_unique<VertexPositionsCommand>();
		for (auto& [index, before] : positionsBefore) {
			const std::vector<Vec3f>& live = meshesAtBegin[index]->VertexPositionList();
			// A size change means topology changed, which this capture cannot undo. Callers use
			// touchMesh for those edits; skipping keeps the history from swapping mismatched arrays.
			if (live.size() == before.size() && live != before) { positions->add(index, std::move(before)); }
		}

		auto composite = std::make_unique<CompositeCommand>(name.empty() ? std::string("Edit") : name);
		bool documentChanged = false;
		if (!transforms->empty()) { composite->add(std::move(transforms)); documentChanged = true; }
		if (!meshes->empty()) { composite->add(std::move(meshes)); documentChanged = true; }
		if (!positions->empty()) { composite->add(std::move(positions)); documentChanged = true; }
		for (auto& cmd : structural) { composite->add(std::move(cmd)); documentChanged = true; }

		if (!SameReferenceImages(imagesBefore, doc.scene.ReferenceImages())) {
			composite->add(std::make_unique<ReferenceImagesCommand>(std::move(imagesBefore)));
			documentChanged = true;
		}
		if (displayBefore != doc.display) {
			composite->add(std::make_unique<DisplaySettingsCommand>(displayBefore));
			documentChanged = true;
		}
		if (selectionBefore != doc.selection.getState()) {
			composite->add(std::make_unique<SelectionCommand>(std::move(selectionBefore)));
		}

		reset();
		if (composite->empty()) { return nullptr; }
		// A click that only changed what is selected is a selection step, whatever tool was active.
		if (!documentChanged) { composite->setLabel("Select"); }
		return composite;
	}

	/// <summary>Puts the document back to how it was at begin() and discards the step.</summary>
	void rollback(DocumentRefs& doc) {
		if (!active) { return; }
		doc.display = displayBefore;
		doc.scene.ReferenceImages() = imagesBefore;
		doc.selection.setState(selectionBefore);
		for (auto it = structural.rbegin(); it != structural.rend(); ++it) { (*it)->undo(doc); }
		for (auto& [index, positions] : positionsBefore) { meshesAtBegin[index]->SwapVertexPositions(positions); }
		for (auto& [index, before] : meshesBefore) { *meshesAtBegin[index] = std::move(before); }
		for (uint i = 0; i < meshesAtBegin.size(); ++i) {
			if (!meshesBefore.count(i)) { transformsAtBegin[i].ApplyTo(*meshesAtBegin[i]); }
		}
		reset();
	}
};

#endif
