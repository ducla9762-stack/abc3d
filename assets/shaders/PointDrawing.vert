#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in uvec2 vertexObjData;

flat out uvec2 ids;

uniform float offset;
uniform mat4 mvp;

void main(){
	gl_Position = mvp * vec4(vertexPosition,1);
	gl_Position.z -= offset;
	ids = vertexObjData;
	ids.y = gl_VertexID - ids.y;
}