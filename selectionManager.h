#ifndef _ABC3D_SELECTION_MANAGER_H_
#define _ABC3D_SELECTION_MANAGER_H_

#include <unordered_map>
#include "helper.h"

#undef max

enum Element { OBJECT, FACE, EDGE, VERTEX };

enum class SelectionTargetType : uint8_t {
	Mesh,
	ReferenceImage
};

// The maximum number of selected elements that is uploaded to the GPU.
constexpr uint MAX_SELECTION_ITEMS = 64;

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

/// <summary>
/// The undoable part of a selection: element mode plus the selected items in order (the first
/// item is the "primary" one the panels show). Additive flags are input state, not document state.
/// </summary>
struct SelectionState {
	Element activeElement = OBJECT;
	std::vector<SelectionItem> items;

	bool operator==(const SelectionState& other) const {
		return activeElement == other.activeElement && items == other.items;
	}
	bool operator!=(const SelectionState& other) const { return !(*this == other); }
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
		if (selectedItems.size() == MAX_SELECTION_ITEMS) { return; }

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

	/// <summary>
	/// Sets the active element type and clears previous selections.
	/// </summary>
	/// <param name="type"> The new selection element type. </param>
	void setActiveElement(Element type) { 
		activeElement = type;
		clearSelection();
	}

	// Additive mode must override the temporary Ctrl-key state.
	bool isAdditive() const { return additive || additiveMode; }
	void setAdditive(bool additive) { this->additive = additive; }

	// Selection logic setting in UI. If additive mode is enabled, selection is
	// always additive even when Ctrl is not pressed.
	bool isAdditiveMode() const { return additiveMode; }
	void setAdditiveMode(bool additiveMode) { this->additiveMode = additiveMode; }

	/// <returns>A copy of the element mode and selected items.</returns>
	SelectionState getState() const { return { activeElement, selectedItems }; }

	/// <summary>
	/// Replaces the element mode and selection with a saved state. Ids are restored as-is;
	/// callers that changed the scene must restore the matching geometry too.
	/// </summary>
	void setState(const SelectionState& state) {
		activeElement = state.activeElement;
		clearSelection();
		for (const SelectionItem& item : state.items) {
			if (!itemMap.count(item)) { addItem(item); }
		}
	}

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
		if (activeElement != OBJECT || objectMap.count(packUints(static_cast<uint>(SelectionTargetType::Mesh), meshId))) { return false; }
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

	// Kept for compatibility with the viewport API: this is the number of
	// distinct selected scene objects, not only mesh objects.
	uint meshCount() const { return static_cast<uint>(objectMap.size()); }
	uint objectCount() const { return static_cast<uint>(objectMap.size()); }

	/// <param name="meshId"> Id of the mesh being queried </param>
	/// <returns> True if contained false otherwise. </returns>
	bool hasSelectedMesh(uint meshId) const {
		return meshMap.count(meshId);
	}

	bool hasSelectedReferenceImage(uint imageId) const {
		return objectMap.count(packUints(static_cast<uint>(SelectionTargetType::ReferenceImage), imageId));
	}

	bool isSelected(uint meshId, uint elementId) const {
		return itemMap.count({ meshId, elementId });
	}
};

#endif
