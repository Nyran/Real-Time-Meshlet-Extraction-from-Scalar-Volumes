// framework internal
#include "OpenGL_impl/opengl_renderer.hpp"



// external libraries: gl3w for loading extensions
#include <GL/gl3w.h>

#include <shaderc/shaderc.hpp>

// C++ standard libraries
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>

#define GET_VARIABLE_NAME(Variable) (#Variable)

// this file contains the functionality to upload the volume data itself and compute, cache, as well as to write/read the min max hierarchy based on the input volume
namespace framework {
	void BaseRenderer::create_task_mesh_shader_based_isosurface_extraction_shader_programs_() {
	}

	void BaseRenderer::create_persistent_isosurface_extraction_shader_programs_() {

		glm::uvec3 const& vol_res = main_volume_descriptor_->resolution;
		glm::uvec3 const& block_size = general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE;
		uint64_t const total_num_blocks =
			static_cast<uint64_t>(std::ceil(vol_res[0] / float(block_size[0]))) *
			static_cast<uint64_t>(std::ceil(vol_res[1] / float(block_size[1]))) *
			static_cast<uint64_t>(std::ceil(vol_res[2] / float(block_size[2])));
		std::string const total_num_blocks_define = "#define TOTAL_NUM_BLOCKS_IN_VOLUME " + std::to_string(total_num_blocks);

		uint32_t const ux = block_size[0];
		uint32_t const uy = block_size[1];
		uint32_t const uz = block_size[2];
		uint32_t const px = ux + 1;
		uint32_t const py = uy + 1;
		uint32_t const pz = uz + 1;

		std::set<std::string> block_size_defines = {
			"#define UNPADDED_WG_SIZE_X " + std::to_string(ux),
			"#define UNPADDED_WG_SIZE_Y " + std::to_string(uy),
			"#define UNPADDED_WG_SIZE_Z " + std::to_string(uz),
			"#define UNPADDED_BLOCK_SIZE_X (UNPADDED_WG_SIZE_X)",
			"#define UNPADDED_BLOCK_SIZE_Y (UNPADDED_WG_SIZE_Y)",
			"#define UNPADDED_BLOCK_SIZE_Z (UNPADDED_WG_SIZE_Z)",
			"#define NUM_UNPADDED_VOXELS_PER_YZ_SLICE " + std::to_string(uy * uz),
			"#define NUM_UNPADDED_VOXELS_PER_XZ_SLICE " + std::to_string(ux * uz),
			"#define NUM_UNPADDED_VOXELS_PER_XY_SLICE " + std::to_string(ux * uy),
			"#define NUM_UNPADDED_VOXELS_PER_BLOCK " + std::to_string(ux * uy * uz),
			"#define PADDED_WG_SIZE_X " + std::to_string(px),
			"#define PADDED_WG_SIZE_Y " + std::to_string(py),
			"#define PADDED_WG_SIZE_Z " + std::to_string(pz),
			"#define PADDED_BLOCK_SIZE_X (PADDED_WG_SIZE_X)",
			"#define PADDED_BLOCK_SIZE_Y (PADDED_WG_SIZE_Y)",
			"#define PADDED_BLOCK_SIZE_Z (PADDED_WG_SIZE_Z)",
			"#define NUM_PADDED_VOXELS_PER_XY_SLICE " + std::to_string(px * py),
			"#define NUM_PADDED_VOXELS_PER_BLOCK " + std::to_string(px * py * pz),
			total_num_blocks_define
		};

		{
			std::map<int32_t, std::set<std::string>> const persistent_extraction_shader_variants =
			{
				{static_cast<int32_t>(framework::PersistentExtractionMode::VOLUME_SPACE_POSITION_AND_NORMAL) + static_cast<int32_t>(framework::PersistentExtractionMode::MAX_OCCUPIED_PMB_CELLS_32),
				{"#define MAX_OCCUPIED_CELLS_MESH_SHADER_PMB 32"},
				},
				{static_cast<int32_t>(framework::PersistentExtractionMode::VOLUME_SPACE_POSITION_AND_NORMAL) + static_cast<int32_t>(framework::PersistentExtractionMode::MAX_OCCUPIED_PMB_CELLS_64),
				{"#define MAX_OCCUPIED_CELLS_MESH_SHADER_PMB 64"},
				},
				{static_cast<int32_t>(framework::PersistentExtractionMode::VOLUME_SPACE_POSITION_AND_NORMAL) + static_cast<int32_t>(framework::PersistentExtractionMode::MAX_OCCUPIED_PMB_CELLS_96),
				{"#define MAX_OCCUPIED_CELLS_MESH_SHADER_PMB 96"},
				},
				{static_cast<int32_t>(framework::PersistentExtractionMode::VOLUME_SPACE_POSITION_AND_NORMAL) + static_cast<int32_t>(framework::PersistentExtractionMode::MAX_OCCUPIED_PMB_CELLS_128),
				{"#define MAX_OCCUPIED_CELLS_MESH_SHADER_PMB 128"},
				}
				

			};


			// === Core pipeline of the EGPGV 2026 paper ===
			// This is the task+mesh-shader pipeline proposed in
			// "Real-Time Meshlet Extraction from Scalar Volumes":
			//   - region_size_analysis.task implements the region analysis stage (Sec. 3.2.1, supplementary Listing 1)
			//   - isosurface_extraction.mesh  implements the meshlet extraction stage (Sec. 3.2.2, supplementary Listing 2)
			// Activated at runtime via --task_mesh_persistent on the command line.
			{
				shader_components mesh_shader_based_persistent_meshlet_isosurface_extraction_shader_components;

				mesh_shader_based_persistent_meshlet_isosurface_extraction_shader_components.task = { "./glsl/task_mesh_shader_based_isosurface_extraction/region_size_analysis.task" };
				mesh_shader_based_persistent_meshlet_isosurface_extraction_shader_components.mesh = { "./glsl/task_mesh_shader_based_isosurface_extraction/isosurface_extraction.mesh" };
				
				std::string DEVICE_DEPENDENT_SUBGROUP_SIZE_DEFINITION = "#define DEVICE_DEPENDENT_SUBGROUP_SIZE ";
				DEVICE_DEPENDENT_SUBGROUP_SIZE_DEFINITION += std::to_string(base_context_->get_subgroup_size());
				std::set<std::string> local_shader_defines = { DEVICE_DEPENDENT_SUBGROUP_SIZE_DEFINITION };
				local_shader_defines.insert(block_size_defines.begin(), block_size_defines.end());
				framework_shader_programs_.persistent.mesh_shader_based_persistent_meshlet_isosurface_extraction_program_ptr = std::make_shared<shader_program>(false, mesh_shader_based_persistent_meshlet_isosurface_extraction_shader_components, persistent_extraction_shader_variants, local_shader_defines);
			}


			std::map<int32_t, std::set<std::string>> const block_proxy_rasterization_shader_variants =
			{
				{static_cast<int32_t>(framework::block_proxy_rasterization_shader_variants_t::PASS_THROUGH_PROXY_COLOR ),
				{"#define PASS_THROUGH_AUX_VIS_COLOR"},
				}
			};
		}



		// === Occupancy preprocessing ===
		// These compute shaders run upstream of the task+mesh extraction pipeline
		// to build the dense list of occupied 8x8x4 blocks for the current isovalue.
		// They are shared between the paper's task+mesh path and the PMB compute fallback.
		{
			// Min/max hierarchy over the volume (one entry per 8x8x4 block).
			shader_components min_max_volume_computation_shader_components;
			min_max_volume_computation_shader_components.compute = { "./glsl/compute_shader_based_isosurface_extraction/min_max_computation_blockwise.comp" };
			framework_shader_programs_.occupancy.min_max_volume_computation_program_ptr = std::make_shared<shader_program>(false, min_max_volume_computation_shader_components, framework::shader_variant_defines_t{}, block_size_defines, false);
		}

		{
			// Dense occupancy buffer: filters blocks straddling the isovalue and produces the
			// compacted list consumed by the region-analysis task shader (Sec. 3.2.1).
			shader_components dense_occupancy_buffer_computation_shader_components;
			dense_occupancy_buffer_computation_shader_components.compute = { "./glsl/compute_shader_based_isosurface_extraction/dense_occupancy_buffer_computation.comp" };
			framework_shader_programs_.occupancy.dense_occupancy_computation_program_ptr = std::make_shared<shader_program>(false, dense_occupancy_buffer_computation_shader_components, framework::shader_variant_defines_t{}, block_size_defines, false);
		}


		// === Compute-shader extraction fallback (Parallel Marching Blocks, reference path) ===
		// Reference implementation that performs both region analysis and meshlet extraction
		// in a single compute shader. Used when --task_mesh_persistent is NOT set; provided as
		// the comparison baseline for the paper's task+mesh approach.
		{
			//parallel_marching_blocks_compute_shader_blockwise_variants_t

			std::map<int32_t, std::set<std::string>> parallel_marching_blocks_blockwise_shader_variants = {
				{ static_cast<int32_t>(framework::parallel_marching_blocks_compute_shader_variants_t::PMB_BLOCKWISE_POSITION__AND_NORMAL__AND_32_BIT_INDICES_PER_BLOCK),
					{"#define SHADER_VARIANT_BLOCKWISE_ENCODING"}
				},
				{ static_cast<int32_t>(framework::parallel_marching_blocks_compute_shader_variants_t::PMB_BLOCKWISE_POSITION__AND_NORMAL__AND_16_BIT_INDICES_PER_BLOCK),
					{"#define SHADER_VARIANT_BLOCKWISE_ENCODING","#define SHADER_VARIANT_LOCAL_16_BIT_INDICES"}
				}
			};

			shader_components extract_persistent_geometry_blocks_PMB_style_shader_components;
			extract_persistent_geometry_blocks_PMB_style_shader_components.compute = { "./glsl/compute_shader_based_isosurface_extraction/parallel_marching_blocks.comp" };
			framework_shader_programs_.occupancy.extract_persistent_geometry_blocks_PMB_style = std::make_shared<shader_program>(false, extract_persistent_geometry_blocks_PMB_style_shader_components, parallel_marching_blocks_blockwise_shader_variants, block_size_defines, false);


			// snapdiv: produces the indirect dispatch parameters for the meshlet forward
			// renderer (number of meshlet groups to draw, derived from the meshlet counter).
			shader_components snapdiv_number_of_occupied_blocks_shader_components;
			snapdiv_number_of_occupied_blocks_shader_components.compute = { "./glsl/compute_shader_based_isosurface_extraction/snapdiv_group_occupied_blocks.comp" };
			framework_shader_programs_.occupancy.snapdiv_number_of_occupied_blocks_program_ptr = std::make_shared<shader_program>(false, snapdiv_number_of_occupied_blocks_shader_components, framework::shader_variant_defines_t{}, block_size_defines, false);
		}


		// for render path: coordinate_system_axis_vis
		{
			shader_components coordinate_system_axes_vis;
			coordinate_system_axes_vis.vertex = { "./glsl/auxiliary_visualizations/coordinate_system_axes_vis.vert" };
			coordinate_system_axes_vis.fragment = { "./glsl/auxiliary_visualizations/coordinate_system_axes_vis.frag" };
			framework_shader_programs_.aux_vis.coordinate_system_axes_vis_program_ptr = std::make_shared<shader_program>(false, coordinate_system_axes_vis);
		}

		

		// for render path: render_main_camera_frustum_
		{
			shader_components render_culling_reference_camera_frustrum_program_ptr_shader_components;
			render_culling_reference_camera_frustrum_program_ptr_shader_components.mesh = { "./glsl/auxiliary_visualizations/culling_reference_camera_frustum_line_vis.mesh" };
			render_culling_reference_camera_frustrum_program_ptr_shader_components.fragment = { "./glsl/auxiliary_visualizations/culling_reference_camera_frustum_line_vis.frag" };
			framework_shader_programs_.aux_vis.render_culling_camera_frustum_program_ptr = std::make_shared<shader_program>(false, render_culling_reference_camera_frustrum_program_ptr_shader_components);
		}


		// for render path: render_volume_boundaries_
		{
			shader_components volume_boundary_vis_shader_components;
			volume_boundary_vis_shader_components.mesh = { "./glsl/auxiliary_visualizations/volume_boundary_vis.mesh" };
			volume_boundary_vis_shader_components.fragment = { "./glsl/auxiliary_visualizations/volume_boundary_vis.frag" };
			framework_shader_programs_.aux_vis.volume_boundary_vis_program_ptr = std::make_shared<shader_program>(false, volume_boundary_vis_shader_components);
		}

		//  indexed mesh vis modes
		{
			//  phong shading
			{
				shader_components forward_rendering_from_VBO_shader_components_phong_shaded;
				forward_rendering_from_VBO_shader_components_phong_shaded.vertex = { "./glsl/forward_rendering/triangle_mode/triangle_forward_rendering_from_VBO_phong.vert" };
				forward_rendering_from_VBO_shader_components_phong_shaded.fragment = { "./glsl/common_shading_models/common_VBO_and_meshlet_buffers_phong.frag" };
				forward_rendering_from_VBO_shader_components_phong_shaded.program_name =
					"::" + std::to_string(__LINE__) + "::"
					+ std::string(GET_VARIABLE_NAME(framework_shader_programs_.forward_rendering.indexed_mesh_forward_rendering_program_ptr_collection))
					+ std::string(GET_VARIABLE_NAME(framework::IndexedMeshVisualizationMode::PHONG_SHADED));

				framework_shader_programs_.forward_rendering.indexed_mesh_forward_rendering_program_ptr_collection[static_cast<int32_t>(framework::IndexedMeshVisualizationMode::PHONG_SHADED)]
					= std::make_shared<shader_program>(false, forward_rendering_from_VBO_shader_components_phong_shaded);
			}
		}

		// === Meshlet forward rendering (paper Sec. 3.3) ===
		// Task+mesh-shader pair that consumes the persistent meshlet/vertex/index buffers
		// produced by the extraction stage above and rasterizes them with optional
		// per-primitive culling (paper Sec. 3.3.1 task shader, Sec. 3.3.2 mesh shader).
		// Variant defines below select between:
		//   - mesh-shader-only vs. task+mesh combination
		//   - our extraction format vs. the meshoptimizer offline-built reference set
		//     (USE_MESHOPTIMIZER_STYLE_RENDERER, used by --reference_meshlets)
		//  meshlet vis modes
		{
			std::map<int32_t, std::set<std::string>> const meshlet_rendering_shader_variant_defines =
			{
				{   
					static_cast<int32_t>(framework::shader_variants_t::MAX_NUM_VERTICES_128_MAX_NUM_TRIANGLES_254),
					{"#define MAX_VERTICES_AND_TRIANGLES_DEFINED_BY_SHADER_VARIANT", "#define MAX_VERTS 128",  "#define MAX_TRIS 254", "#define USE_MESHOPTIMIZER_STYLE_RENDERER"}
				},
					//define shader variant combinations for our meshlet renderer, that does not need an additional 32bit index buffer

				{
				 static_cast<int32_t>(framework::shader_variants_t::MAX_NUM_VERTICES_128_MAX_NUM_TRIANGLES_254)
			   + static_cast<int32_t>(framework::shader_variants_t::RENDER_ON_THE_FLY_EXTRACTED_MESHLETS),
					{"#define MAX_VERTICES_AND_TRIANGLES_DEFINED_BY_SHADER_VARIANT", "#define MAX_VERTS 128",  "#define MAX_TRIS 254"}
				},

				{
				 static_cast<int32_t>(framework::shader_variants_t::TASK_MESH_SHADER_COMBINATION) 
			   + static_cast<int32_t>(framework::shader_variants_t::MAX_NUM_VERTICES_128_MAX_NUM_TRIANGLES_254),
					{"#define TASK_AND_MESH_COMBINATION", "#define MAX_VERTICES_AND_TRIANGLES_DEFINED_BY_SHADER_VARIANT", "#define MAX_VERTS 128",  "#define MAX_TRIS 254", "#define USE_MESHOPTIMIZER_STYLE_RENDERER"}
				},

				{
				 static_cast<int32_t>(framework::shader_variants_t::TASK_MESH_SHADER_COMBINATION)
			   + static_cast<int32_t>(framework::shader_variants_t::MAX_NUM_VERTICES_128_MAX_NUM_TRIANGLES_254) 
			   + static_cast<int32_t>(framework::shader_variants_t::RENDER_ON_THE_FLY_EXTRACTED_MESHLETS),
					{"#define TASK_AND_MESH_COMBINATION", "#define MAX_VERTICES_AND_TRIANGLES_DEFINED_BY_SHADER_VARIANT", "#define MAX_VERTS 128",  "#define MAX_TRIS 254"}
				}
			};


			auto derive_task_shader_blacklist = [](std::map<int32_t, std::set<std::string>> const& meshlet_rendering_shader_defines) -> std::map<int32_t, std::set<shaderc_shader_kind>> {
				std::map<int32_t, std::set<shaderc_shader_kind>> task_shader_blacklist_per_shader_variant;

				for (auto const& [shader_variant_index, shader_variant_definitions] : meshlet_rendering_shader_defines) {
					if (!shader_variant_definitions.contains("#define TASK_AND_MESH_COMBINATION")) {
						task_shader_blacklist_per_shader_variant[shader_variant_index].insert(shaderc_task_shader);
					}
				}

				return task_shader_blacklist_per_shader_variant;
				};

			std::map<int32_t, std::set<shaderc_shader_kind>> const meshlet_rendering_shader_component_blacklist_per_variant_defines = derive_task_shader_blacklist(meshlet_rendering_shader_variant_defines);


			//  phong shading
			{
				std::set<std::string> shading_local_define;
				shading_local_define.insert(block_size_defines.begin(), block_size_defines.end());
				shader_components forward_rendering_from_meshlet_buffers_shader_components_phong;
				forward_rendering_from_meshlet_buffers_shader_components_phong.task = { "./glsl/forward_rendering/triangle_mode/triangle_forward_rendering_from_meshlet_buffers_general_task_and_mesh.task" };
				forward_rendering_from_meshlet_buffers_shader_components_phong.mesh = { "./glsl/forward_rendering/triangle_mode/triangle_forward_rendering_from_meshlet_buffers_general_geometry_setup.mesh" };
				forward_rendering_from_meshlet_buffers_shader_components_phong.fragment= { "./glsl/common_shading_models/common_VBO_and_meshlet_buffers_phong.frag" };
				forward_rendering_from_meshlet_buffers_shader_components_phong.program_name =
					"::" + std::to_string(__LINE__) + "::"
					+ std::string(GET_VARIABLE_NAME(framework_shader_programs_.forward_rendering.meshlet_forward_rendering_program_ptr_collection))
					+ std::string(GET_VARIABLE_NAME(framework::MeshletVisualizationMode::PHONG_SHADED));

				framework_shader_programs_.forward_rendering.meshlet_forward_rendering_program_ptr_collection[static_cast<int32_t>(framework::MeshletVisualizationMode::PHONG_SHADED)]
					= std::make_shared<shader_program>(false, forward_rendering_from_meshlet_buffers_shader_components_phong, meshlet_rendering_shader_variant_defines, shading_local_define, true, meshlet_rendering_shader_component_blacklist_per_variant_defines);
			}
		}

		// Collect all shader programs created in this method and batch-compile them.
		// This maximizes driver-side parallel compilation via ARB_parallel_shader_compile.
		{
			std::vector<std::shared_ptr<shader_program>> all_programs;

			// Helper to add non-null programs
			auto add_if_valid = [&](std::shared_ptr<shader_program> const& p) {
				if (p) all_programs.push_back(p);
			};

			// persistent extraction
			add_if_valid(framework_shader_programs_.persistent.mesh_shader_based_persistent_meshlet_isosurface_extraction_program_ptr);

			// occupancy
			add_if_valid(framework_shader_programs_.occupancy.min_max_volume_computation_program_ptr);
			add_if_valid(framework_shader_programs_.occupancy.dense_occupancy_computation_program_ptr);
			add_if_valid(framework_shader_programs_.occupancy.extract_persistent_geometry_blocks_PMB_style);
			add_if_valid(framework_shader_programs_.occupancy.snapdiv_number_of_occupied_blocks_program_ptr);

			// aux vis
			add_if_valid(framework_shader_programs_.aux_vis.coordinate_system_axes_vis_program_ptr);
			add_if_valid(framework_shader_programs_.aux_vis.render_culling_camera_frustum_program_ptr);
			add_if_valid(framework_shader_programs_.aux_vis.volume_boundary_vis_program_ptr);

			// forward rendering (indexed mesh)
			for (auto& p : framework_shader_programs_.forward_rendering.indexed_mesh_forward_rendering_program_ptr_collection) {
				add_if_valid(p);
			}

			// forward rendering (meshlet)
			for (auto& p : framework_shader_programs_.forward_rendering.meshlet_forward_rendering_program_ptr_collection) {
				add_if_valid(p);
			}

			shader_program::compile_programs_batched(all_programs);
		}
	}
}

#undef GET_VARIABLE_NAME