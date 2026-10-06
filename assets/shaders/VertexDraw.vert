
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 cubePos;
layout(location = 2) in uvec2 ids;

out uvec2 drawIds;

// Represents a mat4 but with access to scale factors and rot matrix.
// 64 bytes in a std140 buffer (due to the 16 byte alignment).
struct CompactTransform140 {
    vec3 col1;
    float x;
    vec3 col2;
    float y;
    vec3 col3;
    float z;
    vec3 scale;
};

layout(std140, binding = 0) uniform ModelTransformBuffer {
    CompactTransform140 model[MAX_OBJECT_COUNT];
} trans;

uniform mat4 viewProj;
uniform float scale;

void main(){
    CompactTransform140 model = trans.model[ids.x];

	vec3 translate = vec3(model.x, model.y, model.z);
    mat3 rot = mat3(model.col1, model.col2, model.col3);

    vec3 worldCubePos = translate + (rot * (model.scale * cubePos));

    vec3 worldVertPos = worldCubePos + (vertPos * scale);

	gl_Position = viewProj * vec4(worldVertPos, 1.0f);
	drawIds = ids;
}
