layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNorm;
layout(location = 2) in uvec2 vertIds;

out vec3 shadePos;
out vec3 normalDir;
flat out uvec2 ids;

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

uniform mat4 view;
uniform mat4 proj;

void main(){

    CompactTransform140 model = trans.model[vertIds.x];

    vec3 translate = vec3(model.x, model.y, model.z);
    mat3 rot = mat3(model.col1, model.col2, model.col3);

    vec3 worldPos = translate + (rot * (model.scale * vertPos)) ;

	vec4 viewPos = view * vec4(worldPos, 1.0f);
	shadePos = vec3(viewPosition);

	gl_Position = proj * viewPos;

    vec3 invScale = vec3(1.0f) / model.scale;
	normalDir = rot * (invScale * vertNorm);

	ids = vertexIds;
}