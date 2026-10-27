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
uniform mat4 mvp;

void main() {
	// Build the quad in world space, then transform it into clip space.
	vec3 worldPos = vec3(positions[gl_VertexID] * scale, 0.0f);
	worldPos.y *= aspectR;
	worldPos += pos;

	gl_Position = mvp * vec4(worldPos, 1.0f);

	texCoord = uv[gl_VertexID];
}
