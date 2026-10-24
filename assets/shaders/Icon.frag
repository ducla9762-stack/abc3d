#version 450 core

out vec4 color;

uniform sampler2D tex;
in vec2 texCoord;

void main(){
	//color = texture(tex, texCoord);
	color = vec4(texCoord, 0.0f, 1.0f);
}