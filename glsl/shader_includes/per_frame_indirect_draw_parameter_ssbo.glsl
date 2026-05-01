#ifndef per_frame_block_based_indirect_draw_parameter_ssbo_GLSL
#define per_frame_block_based_indirect_draw_parameter_ssbo_GLSL

/*
struct draw_elements_indirect_command_struct {
    uint count;
    uint instanceCount;
    uint firstIndex;
    int baseVertex;
    uint baseInstance;
};
*/

// Runtime-sized SSBO. The C++ side computes the actual byte size from
// TOTAL_NUM_BLOCKS_IN_VOLUME and allocates the buffer via glNamedBufferStorage.
// Sizing this array at compile time (with a constant derived from
// TOTAL_NUM_BLOCKS_IN_VOLUME) made shader compilation cost scale as O(N^3)
// in volume resolution.
layout(std430, binding = 9) buffer block_based_draw_elements_indirect_commands_ssbo {
    uint32_t per_frame_indirect_draw_parameter_buffer[];
};

#include "per_frame_indirect_draw_parameter_ssbo_config.h.glsl"

#define num_filtered_blocks_counter (per_frame_indirect_draw_parameter_buffer[NUM_FILTERED_BLOCK_BASED_DRAW_CALL_COMPONENTS_COUNTER_SSBO_UINT32_OFFSET])

#endif //per_frame_block_based_indirect_draw_parameter_ssbo_GLSL
