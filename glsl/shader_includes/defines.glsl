#ifndef DEFINES_GLSL_INCLUDE
#define DEFINES_GLSL_INCLUDE

/*
  Region-size legend (paper Sec. 3.2.1, Fig. 5).

  An extraction block is 8x8x4 cells. The region analysis task shader picks one
  of three nested subblock sizes per block. Each size has an unpadded form (the
  cells the meshlet covers) and a padded form (the surrounding samples needed
  to evaluate marching-cubes cases at the boundary, +1 per dimension):

    SMALLEST_REGION     unpadded 2x2x2  (token 222)   padded 3x3x3  (token 333)
    MEDIUM_SIZED_REGION unpadded 4x4x4  (token 444)   padded 5x5x5  (token 555)
    LARGEST_REGION      unpadded 8x8x4  (token 884)   padded 9x9x5  (token 995)

  These tokens appear throughout the shader code in identifiers such as
  `occupancy_bit_pattern_padding_cells_222`, `dense_occupied_333_or_555_block_indices_and_size_mode`,
  and `num_active_cells_in_555_or_995_area_inclusive_prefix_sum`.
*/

#define M_PI 3.1415926535897932384626433832795

#define LARGEST_REGION (uint8_t(0))
#define MEDIUM_SIZED_REGION (uint8_t(1))
#define SMALLEST_REGION (uint8_t(2))

#define UNPADDED_SMALLEST_REGION_SIZE (ivec3(2,2,2))
#define UNPADDED_SMALLEST_REGION_SIZE_MINUS_ONE (UNPADDED_SMALLEST_REGION_SIZE - ivec3(1))
#define UNPADDED_MEDIUM_SIZED_REGION_SIZE (ivec3(4,4,4))
#define UNPADDED_MEDIUM_SIZED_REGION_SIZE_MINUS_ONE (UNPADDED_MEDIUM_SIZED_REGION_SIZE - ivec3(1))
#define UNPADDED_LARGEST_REGION_SIZE (ivec3(8,8,4))
#define UNPADDED_LARGEST_REGION_SIZE_MINUS_ONE (UNPADDED_LARGEST_REGION_SIZE - ivec3(1))

#define MAX_VERTICES_TO_EXTRACT_MESH_SHADER_PMB (128)
#define MAX_TRIANGLES_TO_EXTRACT_MESH_SHADER_PMB (254)

#endif //DEFINES_GLSL_INCLUDE