#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in uvec3 vertexIds;

out vec3 shadePos;
out vec3 localPos;
out vec3 normalDir;
flat out uvec3 ids;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat3 normalTransform;

void main(){
	localPos = vertexPosition;

	vec4 viewPosition = modelView * vec4(vertexPosition, 1);
	shadePos = viewPosition.xyz;

	gl_Position = projection * viewPosition;

	normalDir = normalTransform * vertexNormal;

	ids = vertexIds;
}