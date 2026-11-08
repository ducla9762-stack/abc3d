#version 450 core

out vec2 pos;

// Local Space Positions
const vec2 positions[4] = vec2[4] (
	vec2(-1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0)
);

const int index[4] = int[4]( 0, 1, 2, 3 );

uniform mat4 mvp;

void main(){
	// World-space position
	pos = positions[index[gl_VertexID]] * 1000.0;

	// Projection of view-space position
	gl_Position = mvp * vec4(pos.x, 0.0, pos.y, 1.0);
}