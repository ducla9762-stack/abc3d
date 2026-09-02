#ifndef _ABC3D_SELECTION_H_
#define _ABC3D_SELECTION_H_

#include "helper.h"

enum Element { OBJECT, FACE, EDGE, VERTEX };

// The maximum number of selected elements that is uploaded to the GPU.
constexpr uint MAX_SELECTION_ITEMS = 64;

struct SelectionItem {
	Element type;
	uint meshId;
	uint componentId;

	bool operator==(const SelectionItem& other) const {
		return (type == other.type && meshId == other.meshId && componentId == other.componentId);
	}
};

#endif
