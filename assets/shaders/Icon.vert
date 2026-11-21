#version 450 core

out vec2 texCoord;

// Local Positions
const vec2 positions[4] = vec2[4] (
	vec2(-1.0f, 1.0f),
	vec2(-1.0f, -1.0f),
	vec2(1.0f, 1.0f),
	vec2(1.0f, -1.0f)
);

const vec2 uv[4] = vec2[4] (
	vec2(0.0f, 1.0f),
	vec2(0.0f, 0.0f),
	vec2(1.0f, 1.0f),
	vec2(1.0f, 0.0f)
);

uniform vec3 pos;
uniform float scale;
uniform float aspectR;

void main(){
	// World-space position
	vec3 vertPos = vec3(positions[gl_VertexID] * scale, 0.0f);
	vertPos.y *= aspectR;
	vertPos += pos;

	// Projection of view-space position
	gl_Position = vec4(vertPos, 1.0f);

	texCoord = uv[gl_VertexID];
}