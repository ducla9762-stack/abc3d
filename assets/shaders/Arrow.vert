#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 base;
layout(location = 2) in vec3 dir;
layout(location = 3) in float length;
layout(location = 4) in float radius;
layout(location = 5) in float arrowTipHeight; // arrow specific.
layout(location = 6) in vec3 clr;
layout(location = 7) in uvec2 ids;

out vec3 drawClr;
out uvec2 drawIds;

uniform mat4 viewProj;
uniform float screenScale;
uniform mat3 gizmoRotation;

const vec3 Y = vec3(0.0f, 1.0f, 0.0f);
const vec3 Z = vec3(0.0f, 0.0f, 1.0f);

void main(){
	vec3 pos = vertexPosition;

	// Adjust the radius of the arrow
	pos *= vec3(radius, 1.0f, radius);

	// Adjust the length of the arrow. Vertices 16-19 are the arrowhead base
	// and vertex 20 is its tip.
	pos.y += float(gl_VertexID > 7) * length;
	pos.y += float(gl_VertexID == 20) * arrowTipHeight;

	// Keep the complete gizmo at a constant screen size. The clip-space w value scales the local geometry by depth.
	vec4 baseClip = viewProj * vec4(base, 1.0f);
	float localHeight = max(length + arrowTipHeight, 0.001f);
	float depthScale = max(baseClip.w, 0.001f) * screenScale / localHeight;
	pos *= depthScale;

	// AI-generated: Rotate the arrow with the selected element, then build its local frame.
	vec3 orientedDir = normalize(gizmoRotation * dir);
	bool dirIsZ = abs(orientedDir.z) > 0.99f;
	vec3 reference = float(dirIsZ) * Y + float(!dirIsZ) * Z;

	mat3 rotation;
	rotation[1] = orientedDir;
	rotation[0] = normalize(cross(orientedDir, reference));
	rotation[2] = cross(rotation[0], orientedDir);

	pos = rotation * pos;
	
	// Move the arrow to desired location in world space
	pos += base;

	// Output final position
	gl_Position = viewProj * vec4(pos, 1.0f);

	// Send data to fragment shader
	drawClr = clr;
	drawIds = ids;
}
