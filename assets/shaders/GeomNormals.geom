#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 viewPos[];
out vec3 shadePos;
out vec3 normalDir;

void main() {

	vec3 faceNormal = cross(viewPos[1] - viewPos[0], viewPos[2] - viewPos[0]);
	faceNormal = normalize(faceNormal);

	for(int i = 0; i < 3; ++i){
		gl_Position = gl_in[i].gl_Position;
		shadePos = viewPos[i];
		normalDir = faceNormal;
		EmitVertex();
	}

}