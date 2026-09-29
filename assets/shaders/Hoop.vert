#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 base;
layout(location = 2) in vec3 dir;
layout(location = 3) in float length;
layout(location = 4) in float radius;
layout(location = 5) in float thickness;
layout(location = 6) in vec3 clr;
layout(location = 7) in uvec2 ids;

out vec3 drawClr;
out uvec2 drawIds;
out vec2 planeCoord;
out float ringThickness;

uniform mat4 viewProj;
uniform float screenScale;

void main(){
	vec3 pos = vertexPosition;
	planeCoord = vertexPosition.xy;

	// Keep the thickness in the plane's local coordinates, so the ring stays the same thickness on screen.
	ringThickness = clamp(thickness, 0.001f, 0.5f);

	// dir is the axis the ring rotates around.
	vec3 normal = normalize(dir);

	// Build plane perpendicular to rotation axis.
	vec3 reference;
	if (abs(normal.y) < 0.99f) {
		reference = vec3(0.0f, 1.0f, 0.0f);
	}
	else {
		reference = vec3(1.0f, 0.0f, 0.0f);
	}
	vec3 tangent = normalize(cross(reference, normal));
	vec3 bitangent = cross(normal, tangent);
	pos = tangent * pos.x + bitangent * pos.y;

	// Scale plane by radius.
	pos *= radius;

	// Keep the complete gizmo at a constant screen size. The clip-space w value scales the local geometry by depth.
	vec4 baseClip = viewProj * vec4(base, 1.0f);
	float depthScale = max(baseClip.w, 0.001f) * screenScale;
	pos *= depthScale;

	// Move the plane to the base location in world space.
	pos += base;

	// Output final position
	gl_Position = viewProj * vec4(pos, 1.0f);

	drawClr = clr;
	drawIds = ids;
}
