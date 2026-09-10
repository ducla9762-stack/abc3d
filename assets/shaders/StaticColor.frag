#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

flat in uvec2 ids;

const uint MAX_SELECTION_ITEMS = 64u;
const uint VERTEX = 3u;
const vec4 HOVER_COLOR = vec4(1.0f, 0.5f, 0.0f, 1.0f);

uniform uint selectionCount;
// Array of selected items.
uniform uvec3 selectedItems[MAX_SELECTION_ITEMS];
uniform uvec3 hoveredItem;

void main(){
	bool selected = false;
	// Loop through all selected items.
	for (uint i = 0u; i < selectionCount; ++i) {
		uvec3 item = selectedItems[i];
		// Only vertex-level selection changes the vertex square color.
		if (item.x == VERTEX && item.y == ids.x && item.z == ids.y) {
			selected = true;
			break;
		}
	}

	bool hovered =
		(hoveredItem.x == VERTEX && hoveredItem.y == ids.x && hoveredItem.z == ids.y);

	// Default purple, selected green, hovered orange.
	color = selected ? vec4(0, 1, 0, 1) : (hovered ? HOVER_COLOR : vec4(1, 0, 1, 1));
	selectIds = uvec3(4294967295, ids);
}
