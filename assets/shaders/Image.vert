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
uniform float imageAspect;
uniform mat4 mvp;

void main() {
	// Build the quad in world space, then transform it into clip space.
	vec2 localPos = positions[gl_VertexID] * scale;
	localPos.x *= imageAspect;
	vec3 worldPos = vec3(localPos, 0.0f);
	worldPos += pos;

	gl_Position = mvp * vec4(worldPos, 1.0f);

	texCoord = uv[gl_VertexID];
}
