#ifndef _ABC3D_SELECTION_MANAGER_H_
#define _ABC3D_SELECTION_MANAGER_H_

#include <algorithm>
#include <optional>

#include "selection.h"

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
	bool processSelection(std::optional<SelectionItem>& item, bool additive) {

		// If the item is null (likely a background hit)
		if (!item) {
			//clearSelection();
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
				
			// If the item is already selected.
			if (contained != selectedItems.end()) {
				// If we are doing additive selection, remove the selected item from the list, retain the rest.
				if (additive) {
					selectedItems.erase(contained);
				}
				// Else we are doing single selection.
				else {
					// If we were previously doing additive selection, clear the list but retain this.
					if (selectedItems.size() > 1) {
						selectedItems.clear();
						selectedItems.push_back(*item);
						return true;
					}
					// Else we want to toggle selection.
					else {
						selectedItems.clear();
					}
				}
				return false;
			}
			// Else the item is new, add it to the list.
			else {
				// If we are doing single selection, clear the old selections and put this one in.
				if (!additive)
				{
					selectedItems.clear();
					selectedItems.push_back(*item);
					return true;
				}
				// Else We are doing multiple selections.
				else {
					// If there are too many selections, don't add to the list.
					if (selectedItems.size() >= MAX_SELECTION_ITEMS) {
						return false;
					}

					// Else, add it to the selection.
					selectedItems.push_back(*item);
					return true;
				}
			}
		}
	}

	/// <summary>
	/// Returns whether or not the mesh at i is selected.
	/// </summary>
	/// <param name="i"> mesh id </param>
	/// <returns> True if contained false otherwise. </returns>
	bool hasSelectedMesh(uint i) {
		return std::any_of(
			selectedItems.begin(),
			selectedItems.end(),
			[i](const SelectionItem& item) {
				return item.meshId == i;
			}
		);
	}
};

#endif
