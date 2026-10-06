#version 450 core

layout(location = 0) out vec4 color;

layout(std430, binding = 0) buffer ColorSSBO
{
    uint clrId[];
};

const vec3 colors[6] = vec3[6] (
    vec3(0.0f, 0.0f, 0.0f), // background color
    vec3(1.0f, 0.0f, 0.0f), // face color
    vec3(1.0f, 1.0f, 0.0f), // edge color
    vec3(1.0f, 0.0f, 1.0f), // vert color
    vec3(0.0f, 1.0f, 0.0f), // select color
    vec3(1.0f, 1.0f, 1.0f) // hover color
);

uniform sampler2D   posTex;
uniform sampler2D   normTex;
uniform usampler2D  idTex;

// 16 reserved values (excluding 0)
const uint MESH_BOUND = 0xfffffff0;

void main(){
    ivec2 texCoord = ivec2(gl_FragCoord);

    vec3 pos    = vec3(texelFetch(posTex, texCoord, 0));
    vec3 norm   = vec3(texelFetch(normTex, texCoord, 0));
    uvec3 id    = uvec3(texelFetch(idTex, texCoord, 0));

    vec3 clr = colors[clrId[id.y]];

    float clrScale = 0.5f * abs(dot(norm, -normalize(pos))) + 0.5f;
    bool isFace = id.z == 1;
    clrScale = clrScale * float(isFace) + 1.0f * float(!isFace);
    clr = clrScale * clr;

    color = vec4(clr, 1.0f);
}

