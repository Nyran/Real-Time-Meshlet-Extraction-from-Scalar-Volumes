void write_position_extension_independent(uint LOCAL_VBO_INDEX, vec4 position_to_write) {
#ifdef USES_EXT_MESH_SHADER_EXTENSION
	gl_MeshVerticesEXT[LOCAL_VBO_INDEX].gl_Position = position_to_write;
#else
	gl_MeshVerticesNV[LOCAL_VBO_INDEX].gl_Position = position_to_write;
#endif 
}

#ifdef PRIMITIVE_TYPE_LINE_LIST
void write_line_primitive_indices_extension_independent(uint LOCAL_PRIMITIVE_INDEX, uvec2 primitive_indices) {
#ifdef USES_EXT_MESH_SHADER_EXTENSION
	gl_PrimitiveLineIndicesEXT[LOCAL_PRIMITIVE_INDEX] = primitive_indices;
#else
	uint base_offset = LOCAL_PRIMITIVE_INDEX * 2;
	gl_PrimitiveIndicesNV[base_offset	 ] = primitive_indices[0];
	gl_PrimitiveIndicesNV[base_offset + 1] = primitive_indices[1];
#endif 
}
#endif//PRIMITIVE_TYPE_LINE_LIST

#ifdef PRIMITIVE_TYPE_TRIANGLE_LIST
void write_triangle_primitive_indices_extension_independent(uint LOCAL_PRIMITIVE_INDEX, uvec3 primitive_indices) {
#ifdef USES_EXT_MESH_SHADER_EXTENSION
	gl_PrimitiveTriangleIndicesEXT[LOCAL_PRIMITIVE_INDEX] = primitive_indices;
#else
	uint base_offset = LOCAL_PRIMITIVE_INDEX * 3;
	gl_PrimitiveIndicesNV[base_offset	 ] = primitive_indices[0];
	gl_PrimitiveIndicesNV[base_offset + 1] = primitive_indices[1];
	gl_PrimitiveIndicesNV[base_offset + 2] = primitive_indices[2];
#endif 
}
#endif//PRIMITIVE_TYPE_TRIANGLE_LIST

void write_num_primitives_extension_independent(uint num_vertices_to_generate, uint num_primitives_to_generate) {
	if (subgroupElect()) {
#ifdef USES_EXT_MESH_SHADER_EXTENSION
	SetMeshOutputsEXT(num_vertices_to_generate, num_primitives_to_generate);
#else
    gl_PrimitiveCountNV = num_primitives_to_generate;
#endif 
	}
}