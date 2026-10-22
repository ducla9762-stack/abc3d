#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 base;
layout(location = 2) in vec3 dir;
layout(location = 3) in float gizmoLength;
layout(location = 4) in float radius;
layout(location = 5) in float thickness;
layout(location = 6) in vec3 clr;
layout(location = 7) in uvec2 ids;

out vec3 drawClr;
out uvec2 drawIds;

uniform mat4 viewProj;
uniform float screenScale;
uniform mat3 gizmoRotation;
uniform float ringSelectionTolerance;

const float UNIT_MAJOR_RADIUS = 0.5f;
const float UNIT_TUBE_RADIUS = 0.05f;
const float AXIS_DEPTH_TIE_BREAK = 0.00005f;

void main(){
	// AI-generated Code:

	// The mesh stores a normalized torus. Rescale only its minor radius so
	// tipSize controls the visible ring thickness without changing its size.
	vec3 pos = vertexPosition;
	vec2 radial = pos.xy;
	float radialLength = max(length(radial), 0.0001f);
	float radialOffset = radialLength - UNIT_MAJOR_RADIUS;
	// The selection-only pass uses a wider torus; the visible pass uses 1.0.
	float tubeScale = max(thickness, 0.001f) * ringSelectionTolerance / UNIT_TUBE_RADIUS;
	radial /= radialLength;
	pos.xy = radial * (UNIT_MAJOR_RADIUS + radialOffset * tubeScale);
	pos.z *= tubeScale;
	pos *= radius;

	// dir is the axis the ring rotates around. Build a stable basis for the
	// ring plane, including when the selected axis is nearly vertical.
	vec3 normal = normalize(gizmoRotation * dir);
	vec3 reference;
	if (abs(normal.y) < 0.99f) {
		reference = vec3(0.0f, 1.0f, 0.0f);
	}
	else {
		reference = vec3(1.0f, 0.0f, 0.0f);
	}
	vec3 tangent = normalize(cross(reference, normal));
	vec3 bitangent = cross(normal, tangent);
	pos = tangent * pos.x + bitangent * pos.y + normal * pos.z;

	// Keep the complete gizmo at a constant screen size.  The clip-space w
	// value scales the local geometry by depth, matching the other gizmos.
	vec4 baseClip = viewProj * vec4(base, 1.0f);
	float depthScale = max(baseClip.w, 0.001f) * screenScale;
	pos *= depthScale;
	pos += base;

	gl_Position = viewProj * vec4(pos, 1.0f);
	// Ring centerlines intersect at equal depth.  Give each axis a tiny,
	// deterministic tie-breaker so those pixels resolve cleanly without
	// changing which ring is actually in front elsewhere.
	gl_Position.z -= float(ids.y) * AXIS_DEPTH_TIE_BREAK * gl_Position.w;
	drawClr = clr;
	drawIds = ids;
}
