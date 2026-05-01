// frustum-culling helpers adapted from the NVIDIA nvpro mesh-shader culling sample:
// https://github.com/nvpro-samples/gl_occlusion_culling/blob/master/cull-common.h

const int CULL_SKIP_ID = ~0;

vec4 getBoxCorner(vec4 bboxMin, vec4 bboxMax, int n)
{
    bvec3 useMax = bvec3((n & 1) != 0, (n & 2) != 0, (n & 4) != 0);
    return vec4(mix(bboxMin.xyz, bboxMax.xyz, useMax), 1);
}

uint getCullBits(vec4 hPos)
{
    uint cullBits = 0;
    cullBits |= hPos.x < -hPos.w ? 1 : 0;
    cullBits |= hPos.x > hPos.w ? 2 : 0;
    cullBits |= hPos.y < -hPos.w ? 4 : 0;
    cullBits |= hPos.y > hPos.w ? 8 : 0;
    cullBits |= hPos.z < -hPos.w ? 16 : 0;
    cullBits |= hPos.z > hPos.w ? 32 : 0;
    cullBits |= hPos.w <= 0 ? 64 : 0;
    return cullBits;
}

vec3 projected(vec4 pos) {
    return pos.xyz / pos.w;
}
