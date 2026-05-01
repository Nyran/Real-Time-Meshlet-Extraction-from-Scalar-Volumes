void write_num_mesh_shader_workgroups_to_emit(uint num_workgroups_to_emit) {
#ifdef USES_EXT_MESH_SHADER_EXTENSION
		EmitMeshTasksEXT(num_workgroups_to_emit, 1, 1);
#else
		gl_TaskCountNV = num_workgroups_to_emit;
#endif //USES_EXT_MESH_SHADER_EXTENSION
}