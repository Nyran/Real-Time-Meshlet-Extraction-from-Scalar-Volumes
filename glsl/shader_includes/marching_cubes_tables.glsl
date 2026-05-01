// GPU side binding of the CPU marching cubes tables
layout (binding = 31) uniform usamplerBuffer edge_table;
layout (binding = 32) uniform usamplerBuffer unique_vertices_per_cell_tri_table;
layout (binding = 33) uniform usamplerBuffer total_num_triangles_per_cell_tri_table;
layout (binding = 34) uniform usamplerBuffer num_unique_vertices_per_cell_table;
layout (binding = 35)  uniform usamplerBuffer packed_16_bit_vertices_tri_table;

//the neighbor mapping fits in const memory and is faster to access in this way compared to explicit buffers
uint8_t neighbor_mapping_table_const_memory[12] = {
    uint8_t(0x00),
    uint8_t(0x11),
    uint8_t(0x08),
    uint8_t(0x01),
    uint8_t(0x04),
    uint8_t(0x15),
    uint8_t(0x0C),
    uint8_t(0x05),
    uint8_t(0x02),
    uint8_t(0x12),
    uint8_t(0x1A),
    uint8_t(0x0A)
};

//row, slice, block per size
const ivec4 SUBBLOCK_SIZE_DIMS[3] = {
    {9, 81, 405, 0}, //LARGE (9x9x5)
    {5, 25, 125, 0}, //MEDIUM
    {3, 9, 27, 0} //SMALL
};


const ivec4 PADDING_CELL_INDICES[3] = {
    {8, 8, 4, 0},
    {4, 4, 4, 0},
    {2, 2, 2, 0}
};

const uint32_t unique_edge_masks[3] = {
    0x1,
    0x8,
    0x100
};