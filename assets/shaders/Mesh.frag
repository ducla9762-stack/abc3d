#version 450 core

in vec3 shadePos;
in vec3 normalDir;
in vec3 edgeDists;
flat in uvec2 ids;
flat in uvec3 edgeIds;

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

const uint MAX_SELECTION_ITEMS = 64u;

uniform uint selectionCount;
// The array of selected items.
uniform uvec3 selectedItems[MAX_SELECTION_ITEMS];
uniform uvec3 hoveredItem;

// uint mapping of the Elements enum.
const uint OBJECT = 0u;
const uint FACE   = 1u;
const uint EDGE   = 2u;
const uint VERTEX = 3u;
const uint NO_HOVER = 4294967295u;
const vec3 HOVER_COLOR = vec3(1.0f, 0.5f, 0.0f);

const vec3 SURFACE_BASE_COLOR = vec3(1, 0, 0);

uint minIndex(in vec3 v){
	uint y = uint((v.y <= v.x) && (v.y <= v.z));
	uint z = uint((v.z < v.x) && (v.z < v.y)) * 2;
	return y + z;
}

// Returns if the selected item is selected.
bool isSelected(uint type, uint objectId, uint componentId) {
	// Loop through the number of selected items.
	for (uint i = 0u; i < selectionCount; ++i) {
		// Check if the selected item is this.
		uvec3 item = selectedItems[i];
		bool sameElement = item.x == type && item.y == objectId;
		bool sameComponent = type == OBJECT || item.z == componentId;

		if (sameElement && sameComponent) {
			return true;
		}
	}

	return false;
}

bool isHovered(uint type, uint objectId, uint componentId) {
	if (hoveredItem.x == NO_HOVER) {
		return false;
	}

	bool sameElement = hoveredItem.x == type && hoveredItem.y == objectId;
	bool sameComponent = type == OBJECT || hoveredItem.z == componentId;
	return sameElement && sameComponent;
}

void main(){
	vec3 normal = normalize(normalDir);
	vec3 viewDir = -normalize(shadePos);

	uvec3 mask = 4294967295 * uvec3(equal(edgeIds, uvec3(4294967295)));
	vec3 edgeDistances = max(edgeDists, mask);

	uint minEdge = minIndex(edgeDistances);
	float edgeDist = edgeDists[minEdge];
	uint edgeId = edgeIds[minEdge];

	selectIds = uvec3(ids, edgeId);
	bool objectSelected = isSelected(OBJECT, ids.x, 0u);
	bool faceSelected = isSelected(FACE, ids.x, ids.y);
	bool edgeSelected = isSelected(EDGE, ids.x, edgeId);
	bool objectHovered = isHovered(OBJECT, ids.x, 0u);
	bool faceHovered = isHovered(FACE, ids.x, ids.y);
	bool edgeHovered = isHovered(EDGE, ids.x, edgeId);

	float threshold = max(abs(dFdx(edgeDists[minEdge])), abs(dFdy(edgeDists[minEdge]))) * 2.0f;
	float AA = threshold * 0.5f;

	vec3 color1 = (objectSelected || edgeSelected) ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 1.0f, 0.0f);
	vec3 color2 = abs(dot(normal, viewDir)) * SURFACE_BASE_COLOR;
	if (objectSelected) {
		color2 = mix(color2, vec3(0.0f, 1.0f, 0.0f), 0.25f);
	}
	if (faceSelected) {
		color2 = mix(color2, vec3(0.0f, 1.0f, 0.0f), 0.65f);
	}
	if (objectHovered && !objectSelected) {
		color2 = mix(color2, HOVER_COLOR, 0.45f);
	}
	if (faceHovered && !faceSelected) {
		color2 = mix(color2, HOVER_COLOR, 0.75f);
	}
	if (edgeHovered && !edgeSelected) {
		color1 = HOVER_COLOR;
	}
	float alpha = 1.0f - smoothstep(threshold - AA, threshold + AA, edgeDist);

	color = vec4(mix(color2, color1, alpha), 1.0f);
}
