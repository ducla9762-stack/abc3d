#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 base;
layout(location = 2) in vec3 dir;
layout(location = 3) in float length;
layout(location = 4) in float radius;
layout(location = 5) in float tipSize; // mace specific.
layout(location = 6) in vec3 clr;
layout(location = 7) in uvec2 ids;

out vec3 drawClr;
out uvec2 drawIds;

uniform mat4 viewProj;
uniform float screenScale;

const vec3 Y = vec3(0.0f, 1.0f, 0.0f);
const vec3 Z = vec3(0.0f, 0.0f, 1.0f);

void main(){
	vec3 pos = vertexPosition;

	// Adjust the radius of the mace
	pos *= vec3(radius, 1.0f, radius);

	// Vertices 8-15 are the shaft's upper ring. Vertices 16-19
	// are the cap bottom and vertices 20-23 are its top.
	pos.y += float(gl_VertexID >= 8) * length;
	if (gl_VertexID >= 20) {
		pos.y = length + tipSize;
	}

	// Keep the complete gizmo at a constant screen size. The clip-space w value scales the local geometry by depth.
	vec4 baseClip = viewProj * vec4(base, 1.0f);
	float localHeight = max(length + tipSize, 0.001f);
	float depthScale = max(baseClip.w, 0.001f) * screenScale / localHeight;
	pos *= depthScale;

	// Rotate the mace
	bool dirIsZ = abs(dir.z) > 0.99f;
	vec3 reference = float(dirIsZ) * Y + float(!dirIsZ) * Z;

	mat3 rotation;
	rotation[1] = dir;
	rotation[0] = normalize(cross(dir, reference));
	rotation[2] = cross(rotation[0], dir);

	pos = rotation * pos;

	// Move the mace to desired location in world space
	pos += base;

	// Output final position
	gl_Position = viewProj * vec4(pos, 1.0f);

	// Send data to fragment shader
	drawClr = clr;
	drawIds = ids;
}
