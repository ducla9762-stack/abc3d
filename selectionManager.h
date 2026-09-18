#ifndef _ABC3D_SELECTION_MANAGER_H_
#define _ABC3D_SELECTION_MANAGER_H_

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
	std::unordered_map<SelectionItem, uint> itemMap;
	std::vector<SelectionItem> selectedItems;
	bool additive = true;

	void deleteItem(const SelectionItem& item) {
		uint itemIndex = itemMap[item];
		itemMap[selectedItems.back()] = itemIndex;

		std::swap(selectedItems[itemIndex], selectedItems.back());
		itemMap.erase(item);
		selectedItems.pop_back();
	}

	void addItem(const SelectionItem& item) {
		if (selectedItems.size() == MAX_SELECTION_ITEMS) { return; }

		itemMap[item] = selectedItems.size();
		selectedItems.push_back(item);
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
	Element getActiveElement() { return activeElement; }

	/// <summary>
	/// Sets the active element type and clears previous selections.
	/// </summary>
	/// <param name="type"> The new selection element type. </param>
	void setActiveElement(Element type) { 
		activeElement = type;
		clearSelection();
	}

	bool isAdditive() { return additive; }
	void setAdditive(bool additive) { this->additive = additive; }

	/// <summary>
	/// Clears the list of selected items.
	/// </summary>
	void clearSelection() {
		itemMap.clear();
		selectedItems.clear();
	}

	/// <summary>
	/// Returns the non-mutable list of selected items.
	/// </summary>
	/// <returns> The address of the list of selected items. </returns>
	const std::vector<SelectionItem>& getSelectedItems() const {
		return selectedItems;
	}

	/// <summary>
	/// Checks a selection item and decides what to do with it.
	/// </summary>
	/// <param name="item"> A selection item at a point. </param>
	/// <param name="additive"> If the selection is additive or single. </param>
	void processSelection(const SelectionItem& item) {
		// if item contained
		if (itemMap.count(item) && (additive || selectedItems.size() == 1)) {
			deleteItem(item);
			return;
		}
		
		if (!additive) {
			clearSelection();
		}
		
		addItem(item);
	}

	/// <param name="meshId"> Id of the mesh being queried </param>
	/// <returns> True if contained false otherwise. </returns>
	/*bool hasSelectedMesh(uint meshId) {
		return itemMap.count({ meshId, uint(0) });
	}*/
};

#endif
