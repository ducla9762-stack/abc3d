#version 450 core

out vec2 pos;

// Local Space Positions
const vec2 positions[4] = vec2[4] (
	vec2(-1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0)
);

uniform mat4 mvp;
uniform vec3 axisU;
uniform vec3 axisV;

void main(){
	// World-space position
	pos = positions[gl_VertexID] * 1000.0;

	// Projection of view-space position
	gl_Position = mvp * vec4(axisU * pos.x + axisV * pos.y, 1.0);
}
