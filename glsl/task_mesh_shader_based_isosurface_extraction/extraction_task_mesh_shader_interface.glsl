#ifndef EXTRACTION_TASK_MESH_SHADER_INTERFACE_GLSL
#define EXTRACTION_TASK_MESH_SHADER_INTERFACE_GLSL

/*
  Task -> mesh shader payload for the persistent extraction pipeline (paper Sec. 3.2,
  Fig. 4 left). One task-shader workgroup analyses one 8x8x4 extraction block, decides
  how many meshlets to emit (1..32), and fills this payload before launching the
  mesh-shader workgroups. Each spawned mesh-shader workgroup reads from the same
  payload and extracts exactly one meshlet from the subblock identified by its
  gl_WorkGroupID.x.

  Indexing convention: where arrays below are sized [32], index i corresponds to the
  i-th meshlet emitted from this block (i in [0, num_emitted_meshlets)).
*/

// inclusive prefix sum of occupied-cell counts over the 5x5x5 / 9x9x5 padded subregion
// per emitted meshlet; the mesh shader recovers its own count via difference of
// adjacent entries (see isosurface_extraction.mesh).
uint32_t num_active_cells_in_555_or_995_area_inclusive_prefix_sum[32];

// flat occupied-cell indices into the padded 9x9x5 area, packed back-to-back across
// all emitted meshlets. For LARGEST_REGION meshlets each 16-bit cell index occupies
// two consecutive uint8 slots (lo/hi); for MEDIUM_SIZED_REGION meshlets a single
// uint8 per cell suffices (5x5x5 = 125 fits in 8 bits). SMALLEST_REGION meshlets
// do not consume slots here -- their cells are enumerated directly in the mesh shader.
uint8_t  occupied_cells_less_than_995[MAX_OCCUPIED_CELLS_MESH_SHADER_PMB * 4];

// global SSBO write offsets reserved by this block's task shader through atomicAdd
// on the corresponding global counters (see atomic_counters.glsl).
uint32_t atomic_meshlet_descriptor_write_offset;  // first meshlet descriptor slot
uint32_t global_extraction_vertex_offset;         // first vertex slot (positions+normals)
uint32_t global_extraction_triangle_offset;       // first triangle slot (in triangles, not indices)

// 1D index of the originating 8x8x4 extraction block (== occupied-block-list entry).
uint32_t baseID;

// per-meshlet local offsets relative to the block's reserved range, packed as
// (vertex_offset << 16) | triangle_offset.
uint32_t local_vertex_and_triangle_offset_per_meshlet[32];

// per-meshlet (subblock_z_order_id_within_block << 0) | (size_mode << 6),
// where size_mode is one of LARGEST_REGION / MEDIUM_SIZED_REGION / SMALLEST_REGION
// (see defines.glsl). The mesh shader uses this to recover its subblock origin
// inside the parent 8x8x4 block.
uint8_t  dense_occupied_333_or_555_block_indices_and_size_mode[32];

#endif //EXTRACTION_TASK_MESH_SHADER_INTERFACE_GLSL