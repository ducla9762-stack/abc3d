#ifndef _ABC3D_SELECTION_MANAGER_H_
#define _ABC3D_SELECTION_MANAGER_H_

#include "helper.h"

enum Element { OBJECT, FACE, EDGE, VERTEX };

struct SelectionItem {
	Element type;
	uint meshId;
	uint componentId;
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
	/// Called on mouse click, checks to see if the given item is a valid selection.
	/// </summary>
	void processClick(std::optional<SelectionItem> item) {
		// If the item is null (likely a background hit)
		if (!item) {
			std::cout << "background" << std::endl;
			clearSelection();
		}
		// Else the item will be valid, check it.
		else {
			std::cout << "type: " << item->type << std::endl;
			// Todo: add to selections list, keep track of that.
		}
	}
};

#endif
