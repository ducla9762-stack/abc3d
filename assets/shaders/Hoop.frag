#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out uvec3 selectIds;

in vec3 drawClr;
in flat uvec2 drawIds;
in vec2 planeCoord;
in float ringThickness;

void main(){
	const float outerRadius = 0.5f;
	float innerRadius = outerRadius - ringThickness;
	float distance = length(planeCoord);

	// Discard the corners of the plane so that we don't hover over them.
	// We don't discard the inner circle so that it is easier to hover over the thin rings.
	if (distance > outerRadius) {
		discard;
	}

	// If inside the circle, no color.
	if (distance < innerRadius) {
		color = vec4(0.0f);
	}
	// Else we color the ring with its specified color.
	else {
		color = vec4(drawClr, 1.0f);
	}
	
	selectIds = uvec3(0xFFFFFFFEu, drawIds.x, drawIds.y);
}
