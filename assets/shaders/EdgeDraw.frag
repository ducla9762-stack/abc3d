layout(location = 2) out uvec3 outIds;

flat in uvec2 drawIds;

void main(){
    outIds = uvec3(drawIds, 2);
}