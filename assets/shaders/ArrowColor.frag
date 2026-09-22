#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

in vec3 drawClr;
in flat uvec2 drawIds;

void main(){
	color = vec4(drawClr, 1.0f);
	selectIds = uvec3(0xFFFFFFFEu, drawIds.x, drawIds.y);
}
