#ifndef FRAMEWORK_AVAILABLE_SHADEAR_PROGRAMS_HPP
#define FRAMEWORK_AVAILABLE_SHADEAR_PROGRAMS_HPP

#include "shader_program.hpp"

#include <array>
#include <memory>

namespace framework {
	using shader_program_ptr = std::shared_ptr<shader_program>;
	template <uint32_t NUM_SHADERS>
	using shader_program_ptr_collection = std::array < std::shared_ptr<shader_program>, NUM_SHADERS>;
	using indexed_mesh_forward_rendering_shader_program_ptr_collection = shader_program_ptr_collection<static_cast<int32_t>(framework::IndexedMeshVisualizationMode::NUM_INDEXED_MESH_SHADER_VIS_PROGRAMS)>;
	using meshlet_forward_rendering_shader_program_ptr_collection = shader_program_ptr_collection<static_cast<int32_t>(framework::MeshletVisualizationMode::NUM_TASK_MESH_SHADER_VIS_PROGRAMS)>;

	struct available_shader_programs_t {
		struct auxiliary_visualization_shader_programs_t {
			shader_program_ptr coordinate_system_axes_vis_program_ptr = nullptr;
			shader_program_ptr render_culling_camera_frustum_program_ptr = nullptr;
			shader_program_ptr volume_boundary_vis_program_ptr = nullptr;
		} aux_vis;

		struct forward_rendering_shader_programs_t {
			indexed_mesh_forward_rendering_shader_program_ptr_collection indexed_mesh_forward_rendering_program_ptr_collection;
			meshlet_forward_rendering_shader_program_ptr_collection meshlet_forward_rendering_program_ptr_collection;
		} forward_rendering;

		struct occupancy_related_volume_computation_shader_prorams_t {
			shader_program_ptr dense_occupancy_computation_program_ptr = nullptr;
			shader_program_ptr extract_persistent_geometry_blocks_PMB_style = nullptr;
			shader_program_ptr min_max_volume_computation_program_ptr = nullptr;
			shader_program_ptr snapdiv_number_of_occupied_blocks_program_ptr = nullptr;
		} occupancy;

		struct persistent_extraction_shader_programs_t {
			shader_program_ptr mesh_shader_based_persistent_meshlet_isosurface_extraction_program_ptr;
		} persistent;
	};
}

#endif //FRAMEWORK_AVAILABLE_SHADEAR_PROGRAMS_HPP