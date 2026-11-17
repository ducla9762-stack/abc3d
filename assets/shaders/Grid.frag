#version 450 core

out vec4 color;
in vec2 pos; // World-Space Position

// Full World-Space Width of Lines (will be a uniform variable later)
float baseLineWidth = 0.01;

// Implemenation of Ben Golus' Pristine Grid Shader:
// https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8

void main(){

	// We use screen space derivatives to approximate the world-space width of the fragment.
	vec2 posDeriv = vec2(length(vec2(dFdx(pos.x), dFdy(pos.x))), length(vec2(dFdx(pos.y), dFdy(pos.y))));

	// Vary lineWidth to differentiate minor lines vs major lines
	// There is probably a better way to do this but this is what I'm doing for now
	bvec2 majorLine = lessThan(5.0 - abs(mod(abs(pos), 10.0) - 5.0), vec2(0.5));
	vec2 lineWidth = baseLineWidth + vec2(majorLine) * baseLineWidth;

	// We shouldn't draw lines smaller than a pixel wide as this will incorporate more aliasing.
	vec2 drawWidth = max(lineWidth, posDeriv);

	// lineAA and lineUV have two components. One corresponds to lines in the x-axis
	// and one corresponds to lines in the z-axis.

	// LineAA is the full world-space width of the anti-aliased edge (smoothstep)
	vec2 lineAA = posDeriv * 1.5; // About 1.5 'pixels' wide

	// LineUV represents a local position on the nearest line edge.
	vec2 lineUVs = 1.0 - abs(fract(pos) * 2.0 - 1.0);

	// We use an inverse smoothstep function for a smooth gradient on the anti-aliased edge.
	// Lines in local UV space are two times larger so this smoothstep is as wide as lineAA in world-space.
	vec2 lines = 1.0 - smoothstep(drawWidth - lineAA, drawWidth + lineAA, lineUVs);

	// We won't draw lines smaller than one pixel wide, but we will fade them out based on how
	// much smaller they are than the desired thickness.
	lines *= clamp(lineWidth / drawWidth, 0.0, 1.0);

	// Combine the alphas computed for each line (one for each axis) with a linear interpolation
	// that is equivalent to an alpha blend.
	float alpha = mix(lines.x, 1.0, lines.y); 

	// Determine color based on whether pixel is on an axis line.
	// There is probably a better way to do this but this is what I'm doing for the moment.
	bvec2 axisLine = bvec2(uvec2(lessThan(abs(pos), vec2(0.5))) * uvec2(greaterThan(lines, vec2(0.0))));
	float r = float(!axisLine.y || axisLine.x);
	float g = float(!axisLine.x || axisLine.y);
	float b = float(!(axisLine.x || axisLine.y));

	// Output final color
	color = vec4(r, g, b, alpha);

	/*
	// Debug Code 

	float lineAA = 0.2;
	float lineUV = abs(pos.x * 2.0);
	float line = 1.0 - smoothstep(lineWidth - lineAA, lineWidth + lineAA, lineUV);

	color = vec4(1.0, 1.0, 1.0, line);

	if(abs(abs(pos.x) - 0.15) < 0.001){
		color = vec4(1.0, 0.0, 0.0, 1.0);
	}

	if(abs(abs(pos.x) - 0.25) < 0.001){
		color = vec4(0.0, 1.0, 0.0, 1.0);
	}

	if(abs(abs(pos.x) - 0.35) < 0.001){
		color = vec4(0.0, 0.0, 1.0, 1.0);
	}
	*/
}