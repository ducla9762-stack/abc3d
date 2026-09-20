#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

flat in uvec2 ids;

const uint MAX_SELECTION_ITEMS = 64u;
const uint OBJECT = 0u;
const uint VERTEX = 3u;
const uint NO_HOVER = 4294967295u;
const vec4 HOVER_COLOR = vec4(1.0f, 0.5f, 0.0f, 1.0f);

uniform uint selectionCount;
// Array of selected items.
uniform uvec3 selectedItems[MAX_SELECTION_ITEMS];
// (element type, object ID, component ID) values under the mouse cursor.
uniform uvec3 hoveredItems[2];

void main(){
	bool selected = false;
	bool hovered = false;
	// Loop through all selected items.
	for (uint i = 0u; i < selectionCount; ++i) {
		uvec3 item = selectedItems[i];
		// If the selected item is this vertex.
		if (item.x == VERTEX && item.y == ids.x && item.z == ids.y) {
			selected = true;
			break;
		}
	}

	// AI-generated: Applies the same hover-state convention to vertex points
	// while preserving the existing selected-versus-unselected colors.
	if (hoveredItems[0].x == OBJECT && hoveredItems[0].y == ids.x) {
		hovered = true;
	}
	else if (hoveredItems[0].x == VERTEX && hoveredItems[0].y == ids.x && hoveredItems[0].z == ids.y) {
		hovered = true;
	}

	// Default purple, selected green, hovered orange.
	color = selected ? vec4(0, 1, 0, 1) :
		hovered && hoveredItems[0].x != NO_HOVER ? HOVER_COLOR :
		vec4(1, 0, 1, 1);
	selectIds = uvec3(4294967295, ids);
}
