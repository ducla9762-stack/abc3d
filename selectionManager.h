#ifndef _ABC3D_SELECTION_MANAGER_H_
#define _ABC3D_SELECTION_MANAGER_H_

#include <unordered_map>
#include "helper.h"

#undef max

enum Element { OBJECT, FACE, EDGE, VERTEX };

// The maximum number of selected elements that is uploaded to the GPU.
constexpr uint MAX_SELECTION_ITEMS = 64;

struct SelectionItem {
	uint meshId;
	uint componentId;

	bool operator==(const SelectionItem& other) const {
		return (meshId == other.meshId && componentId == other.componentId);
	}
};

template<>
struct std::hash<SelectionItem> {
	size_t operator()(const SelectionItem& item) const noexcept { return std::hash<uint64_t>{}(packUints(item.meshId, item.componentId)); }
};

class SelectionManager {
private:
	Element activeElement;
	std::unordered_map<uint, uint> meshMap;
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

		uint meshItemCount = --meshMap[item.meshId];
		if (meshItemCount == 0) {
			meshMap.erase(item.meshId);
		}
	}

	// only call if item is not already added
	void addItem(const SelectionItem& item) {
		if (selectedItems.size() == MAX_SELECTION_ITEMS) { return; }

		itemMap[item] = selectedItems.size();
		selectedItems.push_back(item);

		if (meshMap.count(item.meshId)) {
			++meshMap[item.meshId];
		}
		else {
			meshMap[item.meshId] = 1;
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

	/// <summary>
	/// Clears the list of selected items.
	/// </summary>
	void clearSelection() {
		itemMap.clear();
		meshMap.clear();
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
	}

	/// <summary>
	/// Checks a selection item and decides what to do with it.
	/// </summary>
	/// <param name="item"> A selection item at a point. </param>
	/// <param name="additive"> If the selection is additive or single. </param>
	void processSelection(const SelectionItem& item) {
		const bool effectiveAdditive = isAdditive();

		// if item contained
		if (itemMap.count(item) && (effectiveAdditive || selectedItems.size() == 1)) {
			deleteItem(item);
			return;
		}
		
		if (!effectiveAdditive) {
			clearSelection();
		}
		
		addItem(item);
	}

	void addSelection(const SelectionItem& item) {
		if(!itemMap.count(item)){ addItem(item); }
	}

	uint itemCount() { return (uint) selectedItems.size(); }
	uint itemCount(uint meshId) {
		if (meshMap.count(meshId)) {
			return meshMap[meshId];
		}

		return 0;
	}

	uint meshCount() { return (uint) meshMap.size(); }

	/// <param name="meshId"> Id of the mesh being queried </param>
	/// <returns> True if contained false otherwise. </returns>
	bool hasSelectedMesh(uint meshId) const {
		return meshMap.count(meshId);
	}

	bool isSelected(uint meshId, uint elementId) const {
		return itemMap.count({ meshId, elementId });
	}
};

#endif
