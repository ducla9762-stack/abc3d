#version 450 core

in vec3 shadePos;
in vec3 localPos;
in vec3 normalDir;
flat in uvec3 ids;

layout(location = 0) out vec4 color;
layout(location = 1) out vec3 selectPos;
layout(location = 2) out uvec3 selectIds;

uniform vec3 lightPosition;

const float LIGHT_INTENSITY = 0.5;
const float AMBIENT_INTENSITY = 0.2;
const float ALPHA = 30;
const vec3 SURFACE_COLOR = vec3(1, 0, 0);
const vec3 SPECULAR_COLOR = vec3(1, 1, 1);
const vec3 SELECTION_COLOR = vec3(1, 1, 0);

void main(){
	vec3 normal = normalize(normalDir);
	vec3 lightDirection = normalize(lightPosition - shadePos);
	vec3 halfVector = normalize(lightDirection - normalize(shadePos));
	
	float geometryTerm = max(0, dot(normal, lightDirection));
	vec3 diffuseTerm = geometryTerm * SURFACE_COLOR;
	vec3 specularTerm;

	if(geometryTerm == 0){
		specularTerm = vec3(0);
	} 
	else {
		specularTerm = SPECULAR_COLOR * pow(max(0, dot(halfVector, normal)), ALPHA);
	}

	vec3 ambientTerm = AMBIENT_INTENSITY * SURFACE_COLOR;

	selectPos = localPos;
	selectIds = ids;
}