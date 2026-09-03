#version 450 core

layout(location = 0) out vec4 color;

in vec3 drawClr;
in flat uvec2 drawIds;

void main(){
	color = vec4(drawClr, 1.0f);
}