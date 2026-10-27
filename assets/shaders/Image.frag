#version 450 core

in vec2 texCoord;
out vec4 color;

uniform sampler2D tex;
uniform float opacity;

void main() {
	color = texture(tex, texCoord);
	color.a *= clamp(opacity, 0.0, 1.0);
}
