#version 450 core

in vec2 texCoord;
out vec4 color;

uniform sampler2D tex;
uniform float opacity;
uniform vec3 selectionColor;
uniform int selected;

void main() {
	color = texture(tex, texCoord);
	if (selected != 0) {
		color.rgb = mix(color.rgb, selectionColor, 0.5f);
	}
	color.a *= clamp(opacity, 0.0, 1.0);
}
