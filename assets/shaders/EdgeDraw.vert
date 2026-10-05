
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 p0;
layout(location = 2) in vec3 p1;
layout(location = 3) in uvec2 ids;

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
uniform float radius;

const vec3 Y = vec3(0.0f, 1.0f, 0.0f);
const vec3 Z = vec3(0.0f, 0.0f, 1.0f);

void main(){
    vec3 translate = vec3(model.x, model.y, model.z);
    mat3 rot = mat3(model.col1, model.col2, model.col3);

    vec3 worldP0 = translate + (rot * (model.scale * p0));
    vec3 worldP1 = translate + (rot * (model.scale * p1));

    vec3 edge = worldP0 - worldP1;
    float len = length(edge);
    vec3 dir = normalize(edge);

    vec3 pos = vertPos;
    pos *= radius;
    pos.y += float(gl_VertexID > 7) * length;

    bool dirIsZ = abs(dir.z) > 0.99f;
	vec3 reference = float(dirIsZ) * Y + float(!dirIsZ) * Z;

    mat3 rotation;
	rotation[1] = dir;
	rotation[0] = normalize(cross(dir, reference));
	rotation[2] = cross(rotation[0], dir);

    pos = rotation * pos;
    pos += worldP0;

    gl_Position = viewProj * vec4(pos, 1.0f);

    drawIds = ids;
}