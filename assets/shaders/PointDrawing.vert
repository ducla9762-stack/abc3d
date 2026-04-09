#version 450 core

layout(location = 0) in vec3 vertexPosition;

uniform float scale;

uniform mat4 mvp;

void main(){
	gl_Position = mvp * vec4(vertexPosition*scale,1);
}