#version 450 core

layout(location = 0) out uvec3 selectIds;

uniform uint imageId;

void main()
{
    selectIds = uvec3(0xFFFFFFFDu, imageId, 0u);
}
