#ifndef SSBO_GLSL_INCLUDE
#define SSBO_GLSL_INCLUDE

#include "./defines.glsl"

layout(std430, binding = 40) buffer dense_occupied_block_indices_ssbo {
    uint dense_occupied_block_indices[];
};

layout(std430, binding = 41) buffer dense_meshlet_group_offset_and_length_ssbo {
    uint dense_meshlet_group_offset_and_length[];
};

struct meshlet_geometry_info_t {
    uint v_offset; //global offset into vertex SSBO
    uint t_offset; //global offset into index SSBO in triangles
    uint v_count;  //meshlet vertex count
    uint t_count;  //meshlet index count in triangles, indexing local meshlet buffer
};

layout(std430, binding = 18) buffer meshlet_info_vertex_offset_and_count_index_offset_and_count_ssbo {
    meshlet_geometry_info_t meshlet_geometry_info[];
};

layout(std430, binding = 20) buffer extracted_vertex_positions_ssbo {
    float extracted_vertex_positions[];
};

void write_extracted_vertex_position(uint index, vec3 position) {
    uint32_t base_index = 3 * index;
    for (int i = 0; i < 3; ++i) {
        extracted_vertex_positions[base_index + i] = position[i];
    }
}

vec3 read_extracted_vertex_position(uint index) {
    uint32_t base_index = 3 * index;
    return vec3(extracted_vertex_positions[base_index + 0], extracted_vertex_positions[base_index + 1], extracted_vertex_positions[base_index + 2]);
}

layout(std430, binding = 21) buffer extracted_vertex_normals_ssbo {
    float extracted_vertex_normals[];
};

void write_extracted_vertex_normals(uint index, vec3 normal) {
    uint32_t base_index = 3 * index;
    for (int i = 0; i < 3; ++i) {
        extracted_vertex_normals[base_index + i] = normal[i];
    }
}

vec3 read_extracted_vertex_normals(uint index) {
    uint32_t base_index = 3 * index;
    return vec3(extracted_vertex_normals[base_index + 0], extracted_vertex_normals[base_index + 1], extracted_vertex_normals[base_index + 2]);
}


#ifndef USE_MESHOPTIMIZER_STYLE_RENDERER
layout(std430, binding = 22) buffer extracted_triangle_indices_ssbo {
#if !(defined(SHADER_VARIANT_LOCAL_8_BIT_INDICES) || defined(SHADER_VARIANT_LOCAL_16_BIT_INDICES))
    uint extracted_triangle_indices[];
#else
    #ifdef SHADER_VARIANT_LOCAL_16_BIT_INDICES
    uint16_t extracted_triangle_indices[];
    #endif
    #ifdef SHADER_VARIANT_LOCAL_8_BIT_INDICES
    uint8_t extracted_triangle_indices[];
    #endif
#endif
};
#else //USE_MESHOPTIMIZER_STYLE_RENDERER
layout(std430, binding = 22) buffer extracted_triangle_indices_ssbo {
    uint8_t extracted_triangle_indices[];
};
#endif //USE_MESHOPTIMIZER_STYLE_RENDERER

layout(std430, binding = 23) buffer meshoptimizer_path_meshlet_vertex_base_offset_indices_ssbo {
    uint meshoptimizer_path_meshlet_vertex_base_offset_indices[];
};

struct meshopt_bounds {
    /* bounding sphere, useful for frustum culling */
    vec4 center_and_radius;
};

#ifndef USE_MESHOPTIMIZER_STYLE_RENDERER
layout(std430, binding = 24) buffer extracted_culling_descriptor_ssbo {
    uint extraction_block_id_as_culling_info[];
};
#else //USE_MESHOPTIMIZER_STYLE_RENDERER
layout(std430, binding = 24) buffer extracted_culling_descriptor_ssbo {
    meshopt_bounds meshopt_style_bounds[];
};
#endif //!USE_MESHOPTIMIZER_STYLE_RENDERER

#endif //SSBO_GLSL_INCLUDE