#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in uvec2 vertexIds;
layout(location = 3) in float vertexEdgeDist;
layout(location = 4) in uvec3 vertexEdgeIds;

out vec3 shadePos;
out vec3 normalDir;
out vec3 edgeDists;
flat out uvec2 ids;
flat out uvec3 edgeIds;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat3 normalTransform;

void main(){
	vec4 viewPosition = modelView * vec4(vertexPosition, 1);
	shadePos = viewPosition.xyz;

	gl_Position = projection * viewPosition;

	normalDir = normalTransform * vertexNormal;

	ids = vertexIds;
	edgeIds = vertexEdgeIds;

	edgeDists = vec3(0.0f);
	uint index = gl_VertexID % 3;
	edgeDists[index] = vertexEdgeDist;
}