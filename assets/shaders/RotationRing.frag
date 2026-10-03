#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

in vec3 drawClr;
in flat uvec2 drawIds;

uniform int selectionOnly;

void main(){
	// The wider pass is transparent in the color buffer but still writes the
	// selection ID, giving the thin visible ring a forgiving hit area.
	color = selectionOnly != 0 ? vec4(0.0f) : vec4(drawClr, 1.0f);
	selectIds = uvec3(0xFFFFFFFEu, drawIds.x, drawIds.y);
}
