#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

flat in uvec2 ids;

const uint MAX_SELECTION_ITEMS = 64u;
const uint VERTEX = 3u;

uniform uint selectionCount;
// Array of selected items.
uniform uvec3 selectedItems[MAX_SELECTION_ITEMS];

void main(){
	bool selected = false;
	// Loop through all selected items.
	for (uint i = 0u; i < selectionCount; ++i) {
		uvec3 item = selectedItems[i];
		// If the selected item is this vertex.
		if (item.x == VERTEX && item.y == ids.x && item.z == ids.y) {
			selected = true;
			break;
		}
	}

	// Default purple, selected green.
	color = selected ? vec4(0, 1, 0, 1) : vec4(1, 0, 1, 1);
	selectIds = uvec3(4294967295, ids);
}
