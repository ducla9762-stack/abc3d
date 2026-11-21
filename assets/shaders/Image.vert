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
uniform vec3 scale;
uniform float imageAspect;
uniform vec3 rotation;
uniform mat4 mvp;

mat3 rotationMatrix(vec3 degrees) {
	vec3 r = radians(degrees);
	float cx = cos(r.x), sx = sin(r.x);
	float cy = cos(r.y), sy = sin(r.y);
	float cz = cos(r.z), sz = sin(r.z);
	mat3 rx = mat3(1, 0, 0, 0, cx, -sx, 0, sx, cx);
	mat3 ry = mat3(cy, 0, sy, 0, 1, 0, -sy, 0, cy);
	mat3 rz = mat3(cz, -sz, 0, sz, cz, 0, 0, 0, 1);
	return rz * ry * rx;
}

void main() {
	// Build the quad in world space, then transform it into clip space.
	vec2 localPos = positions[gl_VertexID] * 0.5f;
	localPos.x *= imageAspect * scale.x;
	localPos.y *= scale.y;
	vec3 worldPos = rotationMatrix(rotation) * vec3(localPos, 0.0f);
	worldPos += pos;

	gl_Position = mvp * vec4(worldPos, 1.0f);

	texCoord = uv[gl_VertexID];
}
