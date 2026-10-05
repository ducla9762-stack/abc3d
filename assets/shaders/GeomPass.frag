#version 450 core

layout(location = 0) out vec3 pos;
layout(location = 1) out vec3 norm;
layout(location = 2) out uvec3 outIds;

in vec3 shadePos;
in vec3 normalDir;
flat in uvec2 ids;

void main(){
    pos = shadePos;
    norm = normalize(normalDir);
    outIds = uvec3(ids, 1);
}