
layout(location = 0) in vec3 pos;
layout(location = 7) in uvec2 ids;

out uvec2 drawIds;

// Represents a mat4 but with access to scale factors and rot matrix.
// 64 bytes in a std140 buffer (due to the 16 byte alignment).
struct CompactTransform140 {
    Vec3f col1;
    float x;
    Vec3f col2;
    float y;
    Vec3f col3;
    float z;
    Vec3f scale;
}

layout(std140, binding = 0) uniform trans {
    CompactTransform140 model[MAX_OBJECT_COUNT];
};

uniform mat4 viewProj;
uniform float scale;

void main(){
	vec3 translate = vec3(model.x, model.y, model.z);
    mat3 rot = mat3(model.col1, model.col2, model.col3);

    vec3 worldPos = translate + (rot * (model.scale * (scale * pos)));

	gl_Position = viewProj * viewPos;
	drawIds = ids;
}
