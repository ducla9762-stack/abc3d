#version 450 core

in vec3 normalDir;
in vec3 shadePos;
out vec4 color;

uniform vec3 lightPosition;

const float LIGHT_INTENSITY = 0.5;
const float AMBIENT_INTENSITY = 0.2;
const float ALPHA = 30;
const vec3 SURFACE_COLOR = vec3(1, 0, 0);
const vec3 SPECULAR_COLOR = vec3(1, 1, 1);

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

	// Display diffuse, ambient, and specular components
	color = vec4(LIGHT_INTENSITY * (diffuseTerm + specularTerm) + ambientTerm, 1);

	// Uncomment to just display specular component
	//color = vec4(LIGHT_INTENSITY * specularTerm, 1);

	// Uncomment to just display diffuse + ambient components
	//color = vec4(LIGHT_INTENSITY * diffuseTerm + ambientTerm, 1);
}