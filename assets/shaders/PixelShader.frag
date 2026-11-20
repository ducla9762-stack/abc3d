#version 450 core

layout(location = 0) out vec4 color;

layout(std430, binding = 0) buffer ColorSSBO
{
    uint clrId[];
};

uniform vec3 colors[4];
uniform vec3 blendClrs[3];

uniform sampler2D posTex;
uniform sampler2D normTex;
uniform usampler2D idTex;

const int PIXEL_SIZE = 8;
const float PALETTE_STEPS = 5.0;

void main()
{
    ivec2 framebufferSize = textureSize(idTex, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    ivec2 blockOrigin = (pixel / PIXEL_SIZE) * PIXEL_SIZE;
    ivec2 texCoord = clamp(
        blockOrigin + ivec2(PIXEL_SIZE / 2),
        ivec2(0),
        framebufferSize - ivec2(1)
    );

    uvec3 id = texelFetch(idTex, texCoord, 0).xyz;
    uvec3 exactId = texelFetch(idTex, pixel, 0).xyz;

    // A single block-center sample can miss a thin edge entirely. Keep faces
    // pixelated, but preserve edge and vertex coverage at the actual pixel.
    if (exactId.z >= 2u) {
        texCoord = pixel;
        id = exactId;
    }

    vec3 pos = texelFetch(posTex, texCoord, 0).xyz;
    vec3 norm = normalize(texelFetch(normTex, texCoord, 0).xyz);

    vec3 shadedColor = colors[id.z];

    if (id.z == 1u) {
        float facing = abs(dot(norm, -normalize(pos)));
        float lightBand = facing > 0.75 ? 1.0 : (facing > 0.35 ? 0.68 : 0.38);
        shadedColor *= lightBand;

        float rim = smoothstep(0.45, 0.8, 1.0 - facing);
        vec3 rimColor = min(vec3(1.0), colors[1] * 1.25 + vec3(0.12, 0.08, 0.20));
        shadedColor = mix(shadedColor, rimColor, 0.35 * rim);
    }

    uint blendId = clrId[id.y];
    if (id.z != 0u && blendId != 0u) {
        shadedColor = mix(shadedColor, blendClrs[blendId - 1u], 0.5);
    }

    shadedColor = floor(clamp(shadedColor, 0.0, 1.0) * PALETTE_STEPS + 0.5) / PALETTE_STEPS;
    color = vec4(shadedColor, 1.0);
}
