#version 450 core

layout(location = 0) in vec3 vertexPosition;

out vec3 viewPos;

uniform mat4 modelView;
uniform mat4 projection;

void main(){
	vec4 viewPosition = modelView * vec4(vertexPosition, 1);
	viewPos = viewPosition.xyz;
	gl_Position = projection * viewPosition;
}