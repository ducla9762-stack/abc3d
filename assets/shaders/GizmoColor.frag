#version 450 core

layout(location = 0) out vec4 color;
in vec3 drawClr;

void main() {
	color = vec4(drawClr, 1.0);
}
