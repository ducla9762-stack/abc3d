#version 450 core

in vec3 shadePos;
in vec3 normalDir;
in vec3 edgeDists;
flat in uvec2 ids;
flat in uvec3 edgeIds;

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

const vec3 SURFACE_BASE_COLOR = vec3(1, 0, 0);

uint minIndex(in vec3 v){
	uint y = uint((v.y <= v.x) && (v.y <= v.z));
	uint z = uint((v.z < v.x) && (v.z < v.y)) * 2;
	return y + z;
}

void main(){
	vec3 normal = normalize(normalDir);
	vec3 viewDir = -normalize(shadePos);

	uvec3 mask = 4294967295 * uvec3(equal(edgeIds, uvec3(4294967295)));
	vec3 edgeDistances = max(edgeDists, mask);

	uint minEdge = minIndex(edgeDistances);
	float edgeDist = edgeDists[minEdge];
	uint edgeId = edgeIds[minEdge];

	selectIds = uvec3(ids, edgeId);

	float AA = length(vec2(dFdx(edgeDists[minEdge]), dFdy(edgeDists[minEdge]))) * 1.5f;
	float threshold = 0.07f;

	vec3 color1 = vec3(1.0f, 1.0f, 0.0f);
	vec3 color2 = vec3(abs(dot(normal, viewDir)) * SURFACE_BASE_COLOR);
	float alpha = 1.0f - smoothstep(threshold - AA, threshold + AA, edgeDist);

	color = vec4(mix(color2, color1, alpha), 1.0f);
}