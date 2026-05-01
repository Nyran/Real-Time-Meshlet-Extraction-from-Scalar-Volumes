#ifndef per_frame_indirect_draw_parameter_ssbo_CONFIG_H_GLSL
#define per_frame_indirect_draw_parameter_ssbo_CONFIG_H_GLSL


//5 components for a classic indirect draw call configuration: see OpenGL spec
#define BLOCK_BASED_INDIRECT_DRAW_CALL_COMPONENTS (5u)

#define NUM_BLOCK_BASED_DRAW_CALL_HEADER_SSBO_UINT32_SLOTS (9u)
#define NUM_FILTERED_BLOCK_BASED_DRAW_CALL_COMPONENTS_COUNTER_SSBO_UINT32_OFFSET (0u)

#define BLOCK_BASED_MULTI_DRAW_ELEMENTS_COMMANDS_UINT32_OFFSET (3u)

#define NUM_BLOCK_BASED_DRAW_CALL_SECTIONS_UNTIL_MESHLET_CONFIG_STARTS (3u)

//meshlet indirection is resolved directly within the shaders, we just need to keep the index
#define NUM_MESHLET_BASED_DRAW_CALL_HEADER_SSBO_UINT32_SLOTS (0u)
#define MESHLET_BASED_MULTI_DRAW_ELEMENTS_COMMANDS_UINT32_OFFSET (2u)


#endif //per_frame_indirect_draw_parameter_ssbo_CONFIG_H_GLSL
