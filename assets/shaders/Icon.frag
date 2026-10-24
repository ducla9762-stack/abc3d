#version 450 core

out vec4 color;

uniform sampler2D tex;
in vec2 texCoord;

void main(){
	color = texture(tex, texCoord);
}