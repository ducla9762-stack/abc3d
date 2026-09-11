#version 450 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 base;
layout(location = 2) in vec3 dir;
layout(location = 3) in float length;
layout(location = 4) in float radius;
layout(location = 5) in float arrowTipHeight;
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

	// Adjust the radius of the arrow
	pos *= vec3(radius, 1.0f, radius);

	// Adjust the length of the arrow
	pos.y += float(gl_VertexID > 7) * length;
	pos.y += float(gl_VertexID == 20) * arrowTipHeight;

	// Scale the complete arrow by its distance from the camera so its
	// projected height remains constant in screen space.
	vec4 baseClip = viewProj * vec4(base, 1.0f);
	float localHeight = max(length + arrowTipHeight, 0.001f);
	float depthScale = max(baseClip.w, 0.001f) * screenScale / localHeight;
	pos *= depthScale;

	// Rotate the arrow
	bool dirIsZ = abs(dir.z) > 0.99f;
	vec3 reference = float(dirIsZ) * Y + float(!dirIsZ) * Z;

	mat3 rotation;
	rotation[1] = dir;
	rotation[0] = normalize(cross(dir, reference));
	rotation[2] = cross(rotation[0], dir);

	pos = rotation * pos;
	
	// Move the arrow to desired location in world space
	pos += base;

	// Output final position
	gl_Position = viewProj * vec4(pos, 1.0f);

	// Send data to fragment shader
	drawClr = clr;
	drawIds = ids;
}
