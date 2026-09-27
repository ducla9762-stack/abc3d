#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 base;
layout(location = 2) in vec3 dir;
layout(location = 3) in float length;
layout(location = 4) in float radius;
layout(location = 5) in float size; // cube specific.
layout(location = 6) in vec3 clr;
layout(location = 7) in uvec2 ids;

out vec3 drawClr;
out uvec2 drawIds;

uniform mat4 viewProj;
uniform float screenScale;

const vec3 Y = vec3(0.0f, 1.0f, 0.0f);
const vec3 Z = vec3(0.0f, 0.0f, 1.0f);

void main(){
	vec3 pos = vertexPosition * size;

	// Keep the cube at a stable screen size.
	vec4 baseClip = viewProj * vec4(base, 1.0f);
	float depthScale = max(baseClip.w, 0.001f) * screenScale;
	pos *= depthScale;
	pos += base;

	// Output final position
	gl_Position = viewProj * vec4(pos, 1.0f);

	// Send data to fragment shader
	drawClr = clr;
	drawIds = ids;
}
