#ifndef MATH_GLSL_INCLUDE
#define MATH_GLSL_INCLUDE

#include "defines.glsl"

const ivec2 zero_one = ivec2(0, 1);

vec3 lerp(vec3 a, vec3 b, float t) {
    return a + t * (b-a);
}


vec3 vertex_interp(float isolevel, vec3 p0, vec3 p1, float f0, float f1)
{
    float t = (isolevel - f0) / (f1 - f0);
    return mix(p0, p1, t);
}

// calculate triangle normal
vec3 calc_normal(vec3 v0, vec3 v1, vec3 v2)
{
    vec3 edge0 = v1 - v0;
    vec3 edge1 = v2 - v0;

    return cross(edge0, edge1);
}

uint8_t z_order_884_to_one_d_index(uvec3 three_d_index) {
    return
        uint8_t(
                 ((three_d_index.y & 0x4) << 5) | ((three_d_index.x & 0x4) << 4)
               | ((three_d_index.z & 0x2) << 4) | ((three_d_index.y & 0x2) << 3) | ((three_d_index.x & 0x2) << 2)
               | ((three_d_index.z & 0x1) << 2) | ((three_d_index.y & 0x1) << 1) | (three_d_index.x & 0x1)
               );
}

ivec3 one_d_to_morton_884(uint in_one_d) {
#if 1
    return
    ivec3(mat3(
        (in_one_d & 1), ((in_one_d >> 1) & 1), ((in_one_d >> 2) & 1),
        ((in_one_d >> 3) & 1), ((in_one_d >> 4) & 1), ((in_one_d >> 5) & 1),
        (((in_one_d >> 6) & 1)), (((in_one_d >> 7) & 1)), 0
    )
        * vec3(1, 2, 4))
        ;
#else 
    
    return ivec3(
        (in_one_d       & 1)  + 2 * ((in_one_d >> 3) & 1) + 4 * (((in_one_d >> 6) & 1)),
    ((in_one_d >> 1) & 1) + 2 * ((in_one_d >> 4) & 1) + 4 * (((in_one_d >> 7) & 1)),
    ((in_one_d >> 2) & 1) + 2 * ((in_one_d >> 5) & 1)
    );
#endif
}

ivec3 one_d_to_morton_442(uint in_one_d) {
    return ivec3(
        (in_one_d & 1) + 2 * ((in_one_d >> 3) & 1),
        ((in_one_d >> 1) & 1) + 2 * ((in_one_d >> 4) & 1),
        ((in_one_d >> 2) & 1)
    );
}

ivec3 one_d_to_morton_444(uint in_one_d) {
    return ivec3(
        (in_one_d & 1) + 2 * ((in_one_d >> 3) & 1),
        ((in_one_d >> 1) & 1) + 2 * ((in_one_d >> 4) & 1),
        ((in_one_d >> 2) & 1) + 2 * ((in_one_d >> 5) & 1)
    );
}

ivec3 one_d_to_morton_222(uint in_one_d) {
    return ivec3(
        (in_one_d & 1),
        ((in_one_d >> 1) & 1),
        ((in_one_d >> 2) & 1)
    );
}


ivec3 one_d_to_morton_l1_to_l2(uint in_one_d) {
    return ivec3(
        (in_one_d & 1) + 2 * ((in_one_d >> 3) & 1),
        ((in_one_d >> 1) & 1) + 2 * ((in_one_d >> 4) & 1),
        ((in_one_d >> 2) & 1)
    );
}

// auxiliary functions for flattening 3D indices and unflattening 1D indices
ivec3 unflatten_one_d_index(int one_d_index, ivec2 three_d_extents) {
    ivec3 unflattened_index = ivec3(0, 0, 0);
    unflattened_index.z  = one_d_index / (three_d_extents.y);
    one_d_index         -= unflattened_index.z * three_d_extents.y;
    unflattened_index.y  = one_d_index / three_d_extents.x;
    unflattened_index.x  = one_d_index % three_d_extents.x;
    return unflattened_index;
}

int flatten_three_d_index(ivec3 three_d_index, ivec2 three_d_extents) {
    return three_d_index.x + 
           three_d_index.y * three_d_extents.x + 
           three_d_index.z * three_d_extents.y;
}


// ------------------------------------------------------------
// Utilities
// ------------------------------------------------------------

uint32_t ilog2_pow2(uint32_t n) {
    // Portable fallback (n is small: <=32)
    uint32_t k = 0;
    while ((1u << k) != n) {
        ++k;
    }
    return k;
}

// ------------------------------------------------------------
// Packed (variable-bit) Morton encoders/decoders
//   - Up to 5 bits per axis (sizes up to 32)
//   - "Packed" = we interleave only the existing bit-planes
// ------------------------------------------------------------
uint32_t morton3_encode_packed(uint32_t x, uint32_t y, uint32_t z,
    uint32_t kx, uint32_t ky, uint32_t kz) {
    uint32_t m = 0, p = 0;
    const uint32_t kmax = (kx > ky ? (kx > kz ? kx : kz) : (ky > kz ? ky : kz));
    for (uint32_t b = 0; b < kmax; ++b) {
        if (b < kx) { m |= ((x >> b) & 1u) << p; ++p; }
        if (b < ky) { m |= ((y >> b) & 1u) << p; ++p; }
        if (b < kz) { m |= ((z >> b) & 1u) << p; ++p; }
    }
    return m;
}

uvec3 morton3_decode_packed(uint32_t m,
    uint32_t kx, uint32_t ky, uint32_t kz) {
    uvec3 xyz = { 0, 0, 0 };
    uint32_t p = 0;
    const uint32_t kmax = (kx > ky ? (kx > kz ? kx : kz) : (ky > kz ? ky : kz));
    for (uint32_t b = 0; b < kmax; ++b) {
        if (b < kx) { xyz[0] |= ((m >> p) & 1u) << b; ++p; }
        if (b < ky) { xyz[1] |= ((m >> p) & 1u) << b; ++p; }
        if (b < kz) { xyz[2] |= ((m >> p) & 1u) << b; ++p; }
    }

    return xyz;
}

uint32_t morton2_encode_packed(uint32_t a, uint32_t b,
    uint32_t ka, uint32_t kb) {
    uint32_t m = 0, p = 0;
    const uint32_t kmax = (ka > kb ? ka : kb);
    for (uint32_t bit = 0; bit < kmax; ++bit) {
        if (bit < ka) { m |= ((a >> bit) & 1u) << p; ++p; }
        if (bit < kb) { m |= ((b >> bit) & 1u) << p; ++p; }
    }
    return m;
}

uvec2 morton2_decode_packed(uint32_t m,
    uint32_t ka, uint32_t kb) {

    uvec2 ab = { 0, 0 };
    uint32_t p = 0;
    const uint32_t kmax = (ka > kb ? ka : kb);
    for (uint32_t bit = 0; bit < kmax; ++bit) {
        if (bit < ka) {
            ab[0] |= ((m >> p) & 1u) << bit; ++p;
        }
        if (bit < kb) {
            ab[1] |= ((m >> p) & 1u) << bit; ++p;
        }
    }

    return ab;
}

// ------------------------------------------------------------
// Forward map: linear index -> (x,y,z)
//   UX, UY, UZ are core sizes (powers of two, <=32)
//   Padded domain is [0..UX]x[0..UY]x[0..UZ] (one-sided pad)
//   Order: core  faces x,y,z  edges  corner
// ------------------------------------------------------------
uvec3 index_to_xyz_padded(uint32_t i,
    uint32_t UX, uint32_t UY, uint32_t UZ)
{

    // Preconditions (optional in release builds):
    // - is_pow2(UX) && is_pow2(UY) && is_pow2(UZ)
    // - UX<=32 && UY<=32 && UZ<=32

    const uint32_t kx = ilog2_pow2(UX);
    const uint32_t ky = ilog2_pow2(UY);
    const uint32_t kz = ilog2_pow2(UZ);

    const uint32_t CORE = UX * UY * UZ;

    const uint32_t FX = UY * UZ;           // x=UX face (y,z free)
    const uint32_t FY = UX * UZ;           // y=UY face (x,z free)
    const uint32_t FZ = UX * UY;           // z=UZ face (x,y free)

    if (i < CORE) {
        return morton3_decode_packed(i, kx, ky, kz);
    }

    uint32_t j = i - CORE;


    uvec3 morton_code_3d;
    // ---------------- Faces: x-face  y-face  z-face ----------------
    if (j < (FX + FY + FZ)) {
        uint32_t face, t;
        if (j < FX) { face = 0; t = j; }
        else if (j < FX + FY) { face = 1; t = j - FX; }
        else { face = 2; t = j - FX - FY; }

        // Choose bit-widths for the two free axes
        uint32_t ka = (face == 0) ? ky : kx;    // a: y on x-face, else x
        uint32_t kb = (face == 2) ? ky : kz;    // b: y on z-face, else z

        uvec2 ab = morton2_decode_packed(t, ka, kb); // exactly one call


        // Scatter a,b and fix the face axis to UX/UY/UZ
        if (face == 0) {
            morton_code_3d = uvec3( UX, ab );
        }
        else if (face == 1) {
            morton_code_3d = uvec3( ab[0], UY, ab[1] );
        }
        else {
            morton_code_3d = uvec3( ab, UZ );
        }

        return morton_code_3d;
    }

    // ---------------- Edges (three) ----------------
    j -= (FX + FY + FZ);
    const uint32_t EX = UX, EY = UY, EZ = UZ;   // lengths along free axis

    if (j < EX) {            // (x, UY, UZ)
        return morton_code_3d = uvec3(j, UY, UZ);
    }
    j -= EX;
    if (j < EY) {            // (UX, y, UZ)
        return morton_code_3d = uvec3(UX, j, UZ);
    }
    j -= EY;
    if (j < EZ) {            // (UX, UY, z)
        return morton_code_3d = uvec3(UX, UY, j);
    }

    return uvec3(UX, UY, UZ);
    // ---------------- Corner ----------------
    //x = UX; y = UY; z = UZ;
}

// ------------------------------------------------------------
// Inverse map: (x,y,z) -> linear index
//   Same order as forward; exactly one morton2_encode for faces
// ------------------------------------------------------------
uint32_t xyz_to_index_padded(uint32_t x, uint32_t y, uint32_t z,
    uint32_t UX, uint32_t UY, uint32_t UZ)
{
    const uint32_t kx = ilog2_pow2(UX);
    const uint32_t ky = ilog2_pow2(UY);
    const uint32_t kz = ilog2_pow2(UZ);

    const uint32_t CORE = UX * UY * UZ;
    const uint32_t FX = UY * UZ;
    const uint32_t FY = UX * UZ;
    const uint32_t FZ = UX * UY;

    // Core?
    if (x < UX && y < UY && z < UZ) {
        return morton3_encode_packed(x, y, z, kx, ky, kz);
    }

    // Face/edge/corner flags
    const bool fx = (x == UX);
    const bool fy = (y == UY);
    const bool fz = (z == UZ);
    const uint32_t fsum = int(fx) + int(fy) + int(fz);

    // Faces (exactly one fixed axis), in order x-face, y-face, z-face
    if (fsum == 1) {
        const uint32_t base = CORE + int(fy) * FX + int(fz) * (FX + FY);

        // Select coords and bit-widths without branching on all cases:
        const uint32_t a = fx ? y : x;            // x-face: a=y ; else: a=x
        const uint32_t b = fz ? y : z;            // z-face: b=y ; else: b=z
        const uint32_t ka = fx ? ky : kx;         // bits for a
        const uint32_t kb = fz ? ky : kz;         // bits for b

        return base + morton2_encode_packed(a, b, ka, kb);  // one call
    }

    // Edges (two fixed axes)
    const uint32_t base_edges = CORE + FX + FY + FZ;
    if (fy && fz && x < UX) return base_edges + x;               // (x, UY, UZ)
    if (fx && fz && y < UY) return base_edges + UX + y;          // (UX, y, UZ)
    if (fx && fy && z < UZ) return base_edges + UX + UY + z;     // (UX, UY, z)

    // Corner
    return base_edges + UX + UY + UZ;                            // (UX, UY, UZ)
}






#endif // MATH_GLSL_INCLUDE