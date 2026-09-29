#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 base;
layout(location = 2) in vec3 dir;
layout(location = 3) in float length;
layout(location = 4) in float radius;
layout(location = 5) in float tipSize;
layout(location = 6) in vec3 clr;
layout(location = 7) in uvec2 ids;

out vec3 drawClr;
out uvec2 drawIds;

uniform mat4 viewProj;
uniform float screenScale;

void main(){
	vec3 pos = vertexPosition;
	vec3 normal = normalize(dir);

	// The local quad lies in XY. Build a stable orthonormal basis for the
	// plane perpendicular to the selected rotation axis.
	vec3 reference = abs(normal.y) < 0.99f
		? vec3(0.0f, 1.0f, 0.0f)
		: vec3(1.0f, 0.0f, 0.0f);
	vec3 tangent = normalize(cross(reference, normal));
	vec3 bitangent = cross(normal, tangent);
	pos = tangent * pos.x + bitangent * pos.y;
	pos *= radius;

	// Keep the complete gizmo at a constant screen size. The clip-space w value scales the local geometry by depth.
	vec4 baseClip = viewProj * vec4(base, 1.0f);
	float depthScale = max(baseClip.w, 0.001f) * screenScale;
	pos *= depthScale;

	// Move the plane to its desired location in world space.
	pos += base;

	// Output final position
	gl_Position = viewProj * vec4(pos, 1.0f);

	// Send data to fragment shader
	drawClr = clr;
	drawIds = ids;
}
