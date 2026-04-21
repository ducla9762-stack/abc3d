#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

flat in uvec2 ids;

void main(){
	color = vec4(1, 0, 1, 1);
	selectIds = uvec3(4294967295, ids);
}