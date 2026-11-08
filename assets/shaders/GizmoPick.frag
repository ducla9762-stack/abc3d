#version 450 core

layout(location = 0) out uvec3 selectIds;
flat in uvec2 drawIds;

void main() {
	selectIds = uvec3(0xFFFFFFFEu, drawIds.x, drawIds.y);
}
