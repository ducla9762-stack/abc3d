#ifndef _ABC3D_SELECTION_MANAGER_H_
#define _ABC3D_SELECTION_MANAGER_H_

#include <unordered_map>
#include "scene.h"
#include "helper.h"

#undef max

enum Element { OBJECT, FACE, EDGE, VERTEX };

enum class SelectionTargetType : uint8_t {
	Mesh,
	ReferenceImage
};

struct SelectionItem {
	uint meshId;
	uint componentId;
	SelectionTargetType targetType = SelectionTargetType::Mesh;

	static SelectionItem ReferenceImage(uint imageId) {
		return { MAX_UINT, imageId, SelectionTargetType::ReferenceImage };
	}

	bool isReferenceImage() const { return targetType == SelectionTargetType::ReferenceImage; }
	bool isValid() const { return isReferenceImage() ? componentId != MAX_UINT : meshId != MAX_UINT; }
	uint objectId() const { return isReferenceImage() ? componentId : meshId; }

	bool operator==(const SelectionItem& other) const {
		return meshId == other.meshId && componentId == other.componentId && targetType == other.targetType;
	}
};

template<>
struct std::hash<SelectionItem> {
	size_t operator()(const SelectionItem& item) const noexcept {
		size_t value = std::hash<uint64_t>{}(packUints(item.meshId, item.componentId));
		combineHash(value, std::hash<uint8_t>{}(static_cast<uint8_t>(item.targetType)));
		return value;
	}
};

class SelectionManager {
private:
	Element activeElement;
	std::unordered_map<uint, uint> meshMap;
	std::unordered_map<uint64_t, uint> objectMap;
	std::unordered_map<SelectionItem, uint> itemMap;
	std::vector<SelectionItem> selectedItems;

	bool additive = false;
	// Should override additive so that always adding to selection in this mode.
	bool additiveMode = false;

	// do not call if item is not added
	void deleteItem(const SelectionItem& item) {
		uint itemIndex = itemMap[item];
		itemMap[selectedItems.back()] = itemIndex;

		std::swap(selectedItems[itemIndex], selectedItems.back());
		itemMap.erase(item);
		selectedItems.pop_back();

		const uint64_t objectKey = packUints(static_cast<uint>(item.targetType), item.objectId());
		uint objectItemCount = --objectMap[objectKey];
		if (objectItemCount == 0) {
			objectMap.erase(objectKey);
		}

		if (!item.isReferenceImage()) {
			uint meshItemCount = --meshMap[item.meshId];
			if (meshItemCount == 0) {
				meshMap.erase(item.meshId);
			}
		}
	}

	// only call if item is not already added
	void addItem(const SelectionItem& item) {

		itemMap[item] = selectedItems.size();
		selectedItems.push_back(item);
		++objectMap[packUints(static_cast<uint>(item.targetType), item.objectId())];

		if (!item.isReferenceImage()) {
			if (meshMap.count(item.meshId)) {
				++meshMap[item.meshId];
			}
			else {
				meshMap[item.meshId] = 1;
			}
		}
	}

public:
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="defaultType"> The default element type to select. </param>
	SelectionManager(Element defaultType = OBJECT) : activeElement(defaultType) {}

	/// <summary>
	/// Returns the current element type that is selectable.
	/// </summary>
	/// <returns> The currently active element type. </returns>
	Element getActiveElement() const { return activeElement; }

	void DowngradeObjectSelection(Element newType, const Scene& scene, const std::vector<SelectionItem>& old) {
		switch (newType) {
		case FACE:
		{
			for (const SelectionItem& item : old) {
				if (item.meshId >= scene.MeshCount()) { continue; }

				const HalfEdgeMesh& mesh = *scene[item.meshId];

				SelectionItem faceItem = item;

				for (uint i = 0; i < uint(mesh.FaceCount()); ++i) {
					if (mesh.FaceIsNull(i)) { continue; }
					faceItem.componentId = i;
					if (!itemMap.count(faceItem)) { addItem(faceItem); }
				}
			}
			break;
		}
		case EDGE:
		{
			for (const SelectionItem& item : old) {
				if (item.meshId >= scene.MeshCount()) { continue; }

				const HalfEdgeMesh& mesh = *scene[item.meshId];

				SelectionItem edgeItem = item;

				for (uint i = 0; i < uint(mesh.EdgeCount()); ++i) {
					if (mesh.EdgeIsNull(i)) { continue; }
					edgeItem.componentId = i;
					if (!itemMap.count(edgeItem)) { addItem(edgeItem); }
				}
			}
			break;
		}
		case VERTEX:
		{
			for (const SelectionItem& item : old) {
				if (item.meshId >= scene.MeshCount()) { continue; }

				const HalfEdgeMesh& mesh = *scene[item.meshId];

				SelectionItem vertItem = item;

				for (uint i = 0; i < uint(mesh.VertexCount()); ++i) {
					if (mesh.VertexIsNull(i)) { continue; }
					vertItem.componentId = i;
					if (!itemMap.count(vertItem)) { addItem(vertItem); }
				}
			}
			break;
		}
		}
	}

	void DowngradeFaceSelection(Element newType, const Scene& scene, const std::vector<SelectionItem>& old) {
		std::vector<uint> components; components.reserve(10);
		switch (newType) {
		case EDGE:
		{
			for (const SelectionItem& item : old) {
				if (item.meshId >= scene.MeshCount() || item.componentId >= scene[item.meshId]->FaceCount()) {
					continue;
				}

				scene[item.meshId]->GetFaceLogicalEdges(item.componentId, components);

				SelectionItem edgeItem = item;

				for (uint component : components) {
					edgeItem.componentId = component;
					if (!itemMap.count(edgeItem)) { addItem(edgeItem); }
				}

				components.clear();
			}
			break;
		}
		case VERTEX:
		{
			for (const SelectionItem& item : old) {
				if (item.meshId >= scene.MeshCount() || item.componentId >= scene[item.meshId]->FaceCount()) {
					continue;
				}

				scene[item.meshId]->GetFaceVertices(item.componentId, components);

				SelectionItem vertItem = item;

				for (uint component : components) {
					vertItem.componentId = component;
					if (!itemMap.count(vertItem)) { addItem(vertItem); }
				}

				components.clear();
			}
			break;
		}
		}
	}

	void DowngradeEdgeSelection(const Scene& scene, const std::vector<SelectionItem>& old) {
		for (const SelectionItem& item : old) {
			if (item.meshId >= scene.MeshCount() || item.componentId >= scene[item.meshId]->EdgeCount()) {
				continue;
			}

			HalfEdgeMesh::HalfEdge edge = scene[item.meshId]->GetEdge(item.componentId);

			SelectionItem vertItem = item;

			vertItem.componentId = edge.orig;
			if (!itemMap.count(vertItem)) { addItem(vertItem); }

			vertItem.componentId = edge.dest;
			if (!itemMap.count(vertItem)) { addItem(vertItem); }
		}
	}

	/// <summary>
	/// Sets the active element type and clears previous selections.
	/// </summary>
	/// <param name="type"> The new selection element type. </param>
	void setActiveElement(Element type, const Scene& scene) { 
		if (activeElement == type) { return; }
		if (type < activeElement) { activeElement = type; clearSelection(); return; }

		std::vector<SelectionItem> old;

		itemMap.clear();
		meshMap.clear();
		selectedItems.swap(old);
		
		switch (activeElement) {
		case OBJECT:
			DowngradeObjectSelection(type, scene, old);
			break;
		case FACE:
			DowngradeFaceSelection(type, scene, old);
			break;
		case EDGE:
			DowngradeEdgeSelection(scene, old);
			break;
		}

		activeElement = type;
	}

	// Additive mode must override the temporary Ctrl-key state.
	bool isAdditive() const { return additive || additiveMode; }
	void setAdditive(bool additive) { this->additive = additive; }

	// Selection logic setting in UI. If additive mode is enabled, selection is
	// always additive even when Ctrl is not pressed.
	bool isAdditiveMode() const { return additiveMode; }
	void setAdditiveMode(bool additiveMode) { this->additiveMode = additiveMode; }

	/// <summary>
	/// Clears the list of selected items.
	/// </summary>
	void clearSelection() {
		itemMap.clear();
		meshMap.clear();
		objectMap.clear();
		selectedItems.clear();
	}

	/// <summary>
	/// Returns the non-mutable list of selected items.
	/// </summary>
	/// <returns> The address of the list of selected items. </returns>
	const std::vector<SelectionItem>& getSelectedItems() const {
		return selectedItems;
	}

	bool selectMesh(uint meshId) {
		if (activeElement != OBJECT || meshMap.count(meshId)) { return false; }
		addItem({ meshId, MAX_UINT });
		return true;
	}

	bool selectReferenceImage(uint imageId) {
		if (activeElement != OBJECT || objectMap.count(packUints(static_cast<uint>(SelectionTargetType::ReferenceImage), imageId))) { return false; }
		addItem(SelectionItem::ReferenceImage(imageId));
		return true;
	}

	/// <summary>
	/// Checks a selection item and decides what to do with it.
	/// </summary>
	/// <param name="item"> A selection item at a point. </param>
	/// <param name="additive"> If the selection is additive or single. </param>
	bool processSelection(const SelectionItem& item) {
		if (!item.isValid() || (item.isReferenceImage() && activeElement != OBJECT)) {
			return false;
		}
		const bool effectiveAdditive = isAdditive();

		// if item contained
		if (itemMap.count(item) && (effectiveAdditive || selectedItems.size() == 1)) {
			deleteItem(item);
			return false;
		}
		
		if (!effectiveAdditive) {
			clearSelection();
		}
		
		addItem(item);
		return true;
	}

	void addSelection(const SelectionItem& item) {
		if (item.isValid() && !(item.isReferenceImage() && activeElement != OBJECT) && !itemMap.count(item)) { addItem(item); }
	}

	uint itemCount() { return (uint) selectedItems.size(); }
	uint itemCount(uint meshId) {
		if (meshMap.count(meshId)) {
			return meshMap[meshId];
		}

		return 0;
	}

	uint meshCount() const { return uint(meshMap.size()); }
	uint objectCount() const { return static_cast<uint>(objectMap.size()); }

	/// <param name="meshId"> Id of the mesh being queried </param>
	/// <returns> True if contained false otherwise. </returns>
	bool hasSelectedMesh(uint meshId) const { return meshMap.count(meshId); }

	bool hasSelectedReferenceImage(uint imageId) const {
		return objectMap.count(packUints(static_cast<uint>(SelectionTargetType::ReferenceImage), imageId));
	}

	bool isSelected(uint meshId, uint elementId) const { return itemMap.count({ meshId, elementId }); }
};

#endif
