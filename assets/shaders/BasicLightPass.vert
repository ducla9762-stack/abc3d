#version 450 core

const vec2 frame_pos[4] = vec2[4] (
	vec2(-1.0f, 1.0f),
	vec2(-1.0f, -1.0f),
	vec2(1.0f, 1.0f),
	vec2(1.0f, -1.0f)
);

const vec2 frame_uv[4] = vec2[4] (
	vec2(0.0f, 1.0f),
	vec2(0.0f, 0.0f),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0f)
);

const int index[4] = int[4]( 0, 1, 2, 3 );

// Ended up not needing the uv since we can do a direct texel fetch
// out vec2 uv;

void main(){
    int i = index[gl_VertexID];
    gl_Position = vec4(frame_pos[i], 0.0f, 1.0f);
    // uv = frame_uv[i];
}