#version 450 core

layout(location = 0) out vec4 color;

layout(std430, binding = 0) buffer
{
    bool clrId[];
};

const vec3 colors[5] = vec3[5] (
    vec3(0.0f, 0.0f, 0.0f); // background color
    vec3(1.0f, 0.0f, 0.0f); // face color
    vec3(1.0f, 0.0f, 1.0f); // vert color
    vec3(1.0f, 1.0f, 0.0f); // edge color
    vec3(0.0f, 1.0f, 0.0f); // select color
    vec3(1.0f, 1.0f, 1.0f); // hover color
);

uniform sampler2D   posTex;
uniform sampler2D   normTex;
uniform usampler2D  idTex;

// 16 reserved values (excluding 0)
const uint MESH_BOUND = 0xfffffff0;

main(){
    ivec2 texCoord = ivec2(gl_FragCoord);

    vec3 pos    = vec3(texelFetch(posTex, texCoord));
    vec3 norm   = vec3(texelFetch(normTex, texCoord));
    uvec2 id    = uvec2(texelFetch(idTex, texCoord)).y;

    vec3 clr = clrId[id.y];

    float clrScale = min(abs(dot(norm, -normalize(shadePos))), 0.2f);
    clrScale *= float(id.x != 0 && id.x < MESH_BOUND);
    clr = clrScale * clr;

    color = vec4(clr, 1.0f);
}

