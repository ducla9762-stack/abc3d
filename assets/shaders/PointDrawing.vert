#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in uvec2 vertexObjData;

flat out uvec2 ids;

uniform float scale;
uniform mat4 mvp;

void main(){
	gl_Position = mvp * vec4(vertexPosition*scale,1);
	ids = vertexObjData;
	ids.y = gl_VertexID - ids.y;
}