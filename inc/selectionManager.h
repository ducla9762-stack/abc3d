#ifndef _ABC3D_SELECTION_MANAGER_H_
#define _ABC3D_SELECTION_MANAGER_H_

#include "helper.h"

enum Element { OBJECT, FACE, EDGE, VERTEX };

struct SelectionItem {
	Element type;
	uint meshId;
	uint componentId;

	bool operator==(const SelectionItem& other) const {
		return (type == other.type && meshId == other.meshId && componentId == other.componentId);
	}
};

class SelectionManager {
private:
	Element activeElement;
	std::vector<SelectionItem> selectedItems;

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

	/// <summary>
	/// Clears the list of selected items.
	/// </summary>
	void clearSelection() {
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
	/// <returns> True if added false if not. </returns>
	bool processSelection(std::optional<SelectionItem>& item, boolean additive) {

		// If the item is null (likely a background hit)
		if (!item) {
			clearSelection();
			return false;
		}
		// This shouldn't happen.
		if (item->type != activeElement)
		{
			return false;
		}

		// Else a valid item has been selected.
		else {
			auto contained = std::find(selectedItems.begin(), selectedItems.end(), *item);
				
			// If the item already exists, remove it from the list.
			if (contained != selectedItems.end()) {
				// If single selection.
				if (!additive) {
					selectedItems.clear();
					return false;
				}
				// Else additive selection.
				else {
					selectedItems.erase(contained);
					return false;
				}
			}
			// Else the item is new, add it to the list.
			else {
				if (!additive)
				{
					selectedItems.clear();
					selectedItems.push_back(*item);
					return true;
				}
				else {
					selectedItems.push_back(*item);
					return true;
				}
			}
		}
	}
};

#endif
