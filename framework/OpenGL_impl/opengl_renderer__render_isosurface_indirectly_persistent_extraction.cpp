#include "opengl_renderer.hpp"

#include "../glsl/shader_includes/atomic_counter_buffer_config.h.glsl"
#include "../glsl/shader_includes/per_frame_indirect_draw_parameter_ssbo_config.h.glsl"

#include <GL/gl3w.h>
#include <glm/gtx/string_cast.hpp>

#include <bitset>
#include <fstream>
#include <numeric>

namespace framework {

	void OpenGLRenderer::extraction_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_() {
		
		
		assert(nullptr != main_volume_descriptor_);

		clear_atomic_counter_buffer_();

		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__dense_occupancy_computation_timer_->start();
		}
		compute_dense_occupancy_brick_indices_();

		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__dense_occupancy_computation_timer_->end();
		}


		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__extraction_times_isosurface_extraction_timer_->start();
		}


		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT );

		auto currently_active_extraction_shader = static_cast<framework::shader_program_ptr>(framework_shader_programs_.occupancy.extract_persistent_geometry_blocks_PMB_style);



		uint32_t extraction_variant_mode = 0;

		//PASS_EXTRACTION_REGION_SIZE_COLOR
		bool meshlet_conformant_encoding = false;
		uint32_t num_byte_per_extracted_triangle_index_ = 4u;
		switch (renderer_conf.indirect_rendering_compute.compute_shader_path_extraction_mode) {
		case ComputeShaderExtractionMode::SINGLE_INDEX_ARRAY:
			extraction_variant_mode = static_cast<int32_t>(framework::parallel_marching_blocks_compute_shader_variants_t::PMB_POSITION__AND_NORMAL__AND_LARGE_32_BIT_INDEX_ARRAY);
			break;

		case ComputeShaderExtractionMode::BLOCKWISE_32BIT_INDICES:
			extraction_variant_mode = static_cast<int32_t>(framework::parallel_marching_blocks_compute_shader_variants_t::PMB_BLOCKWISE_POSITION__AND_NORMAL__AND_32_BIT_INDICES_PER_BLOCK);
			break;

		case ComputeShaderExtractionMode::BLOCKWISE_16BIT_INDICES:
			extraction_variant_mode = static_cast<int32_t>(framework::parallel_marching_blocks_compute_shader_variants_t::PMB_BLOCKWISE_POSITION__AND_NORMAL__AND_16_BIT_INDICES_PER_BLOCK);
			num_byte_per_extracted_triangle_index_ = 2u;
			break;
		}


		//use and indirectly dispatch extraction compute shader
		currently_active_extraction_shader->use(extraction_variant_mode);
		currently_active_extraction_shader->dispatch_indirect(0);
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);


		//collect and prepare indirect arguments for following draw calls
		framework_shader_programs_.occupancy.snapdiv_number_of_occupied_blocks_program_ptr->use();
		framework_shader_programs_.occupancy.snapdiv_number_of_occupied_blocks_program_ptr->dispatch(1, 1, 1);
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);


		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__extraction_times_isosurface_extraction_timer_->end();
		}


	}


	void OpenGLRenderer::extraction_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_() {

		assert(nullptr != main_volume_descriptor_);

		clear_atomic_counter_buffer_(false);

		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__dense_occupancy_computation_timer_ ->start();
		}
		compute_dense_occupancy_brick_indices_();

		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__dense_occupancy_computation_timer_->end();
		}

		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__extraction_times_isosurface_extraction_timer_->start();
		}


		
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

		int32_t const mesh_shader_based_extraction_shader_variant = get_mode_dependent_mesh_shader_extraction_shader_variant();

		const int32_t shader_variant_mode = mesh_shader_based_extraction_shader_variant;

		//turn of rasterizer because we will use our task-mesh-based isosurface extraction shader pipeline (cmp. left-hand side of Figure 4) purely for extracting meshlets from a scalar volume
		glEnable(GL_RASTERIZER_DISCARD);


		framework_shader_programs_.persistent.mesh_shader_based_persistent_meshlet_isosurface_extraction_program_ptr->use(shader_variant_mode);

		//ensure collapsed_atomic_counter_buffers is bound as GL_DRAW_INDIRECT_BUFFER
		//so that glDrawMeshTasksIndirectNV reads the indirect launch params from the correct buffer
		if (currently_bound_indirect_draw_buffer != collapsed_atomic_counter_buffers) {
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, collapsed_atomic_counter_buffers);
			currently_bound_indirect_draw_buffer = collapsed_atomic_counter_buffers;
		}

		if (use_NV_mesh_shader_extension_) {
			opengl_context_->loaded_api_extensions.glDrawMeshTasksIndirectNV(0);
		}
		else {
			//NOT YET SUPPORTED ON NVIDIA HARDWARE
			opengl_context_->loaded_api_extensions.glDrawMeshTasksIndirectEXT(0);
		}


		glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

		glDisable(GL_RASTERIZER_DISCARD);



		framework_shader_programs_.occupancy.snapdiv_number_of_occupied_blocks_program_ptr->use();
		framework_shader_programs_.occupancy.snapdiv_number_of_occupied_blocks_program_ptr->dispatch(1, 1, 1);
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);



		num_byte_per_extracted_triangle_index_ = 1;

		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__extraction_times_isosurface_extraction_timer_->end();
		}
	}


	void OpenGLRenderer::create_offline_optimized_meshlets_from_extracted_isosurface() {

		uint32_t read_back_vertex_count = 0x0;
		glGetNamedBufferSubData(parallel_marching_blocks_extracted_vertices_atomic_counter, NUM_EXTRACTED_VERTICES_ATOMIC_COUNTER_SSBO_UINT32_OFFSET * sizeof(uint32_t), sizeof(uint32_t), &read_back_vertex_count);
		uint32_t read_back_triangle_count = 0x0;
		glGetNamedBufferSubData(parallel_marching_blocks_extracted_indices_atomic_counter, NUM_EXTRACTED_INDICES_ATOMIC_COUNTER_SSBO_UINT32_OFFSET * sizeof(uint32_t), sizeof(uint32_t), &read_back_triangle_count);

		uint32_t read_back_index_count = read_back_triangle_count * 3;

		glGetNamedBufferSubData(persistent_extraction_pos_VBO_, 0, read_back_vertex_count * sizeof(float)*3, positions_cpu_.data());
		glGetNamedBufferSubData(persistent_extraction_nor_VBO_, 0, read_back_vertex_count * sizeof(float)*3, normals_cpu_.data());
		glGetNamedBufferSubData(persistent_extraction_EBO__indices, 0, read_back_index_count * sizeof(uint32_t), indices_cpu_.data());

		
		build_meshlets_offline(read_back_vertex_count, read_back_index_count);

		auto deduplicated_vertex_count = read_back_vertex_count;
		auto deduplicated_index_count = read_back_index_count;

		glNamedBufferSubData(persistent_extraction_pos_VBO_, 0, deduplicated_vertex_count * sizeof(float)*3, remapped_positions_.data());
		glNamedBufferSubData(persistent_extraction_nor_VBO_, 0, deduplicated_vertex_count * sizeof(float)*3, remapped_normals_.data());
		glNamedBufferSubData(persistent_extraction_EBO__indices, 0, meshlet_triangle_indices_.size() * sizeof(uint8_t), meshlet_triangle_indices_.data());

		glNamedBufferSubData(meshoptimizer_created_meshlet_vertex_indices_buffer_32bit_, 0, meshlet_vertex_indices_.size() * sizeof(uint32_t), meshlet_vertex_indices_.data());
		glNamedBufferSubData(persistent_extraction_meshlet_vertex_offset_and_count__index_offset_and_count__buffer_, 0, sizeof(::meshopt_Meshlet)* num_offline_created_meshlets_, built_meshlets_cpu_.data());
		glNamedBufferSubData(meshlet_culling_info_buffer_, 0, sizeof(glm::vec4) * num_offline_created_meshlets_, meshlet_bounding_spheres_.data());
		glNamedBufferSubData(collapsed_atomic_counter_buffers, 5 * sizeof(uint32_t), sizeof(uint32_t), &num_offline_created_meshlets_);

		num_byte_offline_built_meshlets_
			= deduplicated_vertex_count * sizeof(float) * 3 * 2
			+ meshlet_triangle_indices_.size()
			+ meshlet_vertex_indices_.size() * sizeof(uint32_t)
			+ num_offline_created_meshlets_ * sizeof(uint32_t) * 4
			+ num_offline_created_meshlets_ * 11 * sizeof(float);

		num_byte_per_extracted_triangle_index_ = 1;
	}




	void OpenGLRenderer::internal_render_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_() {
		framework_shader_programs_.forward_rendering.indexed_mesh_forward_rendering_program_ptr_collection[static_cast<int32_t>(renderer_conf.isosurface.persistent_indexed_mesh_visualization_mode)]->use();

		glBindVertexArray(VAO_solid_SOA_with_index_buffer__position_and_normal);


		uint32_t const BYTE_OFFSET_TO_COUNT_VARIABLE = 0;
		uint32_t const BYTE_OFFSET_INTO_DRAW_COMMAND_BUFFER = NUM_BLOCK_BASED_DRAW_CALL_HEADER_SSBO_UINT32_SLOTS * sizeof(uint32_t);

		switch (renderer_conf.indirect_rendering_compute.compute_shader_path_extraction_mode) {
		case ComputeShaderExtractionMode::SINGLE_INDEX_ARRAY:
		{
			uint32_t read_back_index_draw_count = 0x0;
			if (currently_bound_indirect_draw_buffer != collapsed_atomic_counter_buffers) {
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, collapsed_atomic_counter_buffers);
				currently_bound_indirect_draw_buffer = collapsed_atomic_counter_buffers;
			}

			glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)(SINGLE_INDEXED_ARRAY_INDIRECT_DRAW_COMMAND_SSBO_UINT32_OFFSET * sizeof(uint32_t)));
			break;
		}
		//we skip the first block because there we encode our mesh and task shader indirect draw call
		case ComputeShaderExtractionMode::BLOCKWISE_32BIT_INDICES:
			glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)(BYTE_OFFSET_INTO_DRAW_COMMAND_BUFFER), BYTE_OFFSET_TO_COUNT_VARIABLE, overall_num_blocks_in_volume_, sizeof(DrawElementsIndirectCommand));
			num_byte_per_extracted_triangle_index_ = 4;
			break;

		case ComputeShaderExtractionMode::BLOCKWISE_16BIT_INDICES:
			glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_SHORT, (const void*)(BYTE_OFFSET_INTO_DRAW_COMMAND_BUFFER), BYTE_OFFSET_TO_COUNT_VARIABLE, overall_num_blocks_in_volume_, sizeof(DrawElementsIndirectCommand));
			num_byte_per_extracted_triangle_index_ = 2;
			break;
		}

		if (currently_bound_indirect_draw_buffer != collapsed_atomic_counter_buffers) {
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, collapsed_atomic_counter_buffers);
			currently_bound_indirect_draw_buffer = collapsed_atomic_counter_buffers;
		}

	}

	void OpenGLRenderer::internal_render_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_() {
		int32_t meshlet_forward_rendering_shader_variant = get_mode_dependent_meshlet_forward_rendering_shader_variant();

		framework_shader_programs_.forward_rendering.meshlet_forward_rendering_program_ptr_collection[static_cast<int32_t>(renderer_conf.isosurface.persistent_meshlet_visualization_mode)]
			->use(
				meshlet_forward_rendering_shader_variant
			);

		if (currently_bound_indirect_draw_buffer != collapsed_atomic_counter_buffers) {
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, collapsed_atomic_counter_buffers);
			currently_bound_indirect_draw_buffer = collapsed_atomic_counter_buffers;
		}

		if (renderer_conf.isosurface.meshlet_forward_rendering_pipeline_mode == MeshletForwardRenderPipelineMode::MESH_SHADER_ONLY_NO_CULLING) {
			opengl_context_->loaded_api_extensions.glDrawMeshTasksIndirectNV(5 * sizeof(uint32_t));
		}
		else {
			opengl_context_->loaded_api_extensions.glDrawMeshTasksIndirectNV(1018 * sizeof(uint32_t));
		}
	}

	void OpenGLRenderer::render_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_() {

		if (!renderer_conf.general.render_isosurface) {
			return;
		}

		assert(nullptr != main_volume_descriptor_);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);


		if (currently_bound_indirect_draw_buffer != per_frame_block_based_indirect_draw_parameter_ssbo_) {
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, per_frame_block_based_indirect_draw_parameter_ssbo_);
			currently_bound_indirect_draw_buffer = per_frame_block_based_indirect_draw_parameter_ssbo_;
		}

		internal_render_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_();
	}



	void OpenGLRenderer::render_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_() {
		if (!renderer_conf.general.render_isosurface) {
			return;
		}
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		internal_render_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_();
	}

	void OpenGLRenderer::render_pass_isosurface_indirectly_persistent_extraction_offline_created_meshlet_based_() {
		if (!renderer_conf.general.render_isosurface) {
			return;
		}
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		int32_t meshlet_forward_rendering_shader_variant = get_mode_dependent_meshlet_forward_rendering_shader_variant(true);
		framework_shader_programs_.forward_rendering.meshlet_forward_rendering_program_ptr_collection[static_cast<int32_t>(renderer_conf.isosurface.persistent_meshlet_visualization_mode)]
			->use(
				meshlet_forward_rendering_shader_variant
			);

		if (num_offline_created_meshlets_ > 0) {
			if (renderer_conf.isosurface.meshlet_forward_rendering_pipeline_mode == MeshletForwardRenderPipelineMode::MESH_SHADER_ONLY_NO_CULLING) {
				opengl_context_->loaded_api_extensions.glDrawMeshTasksNV(0, num_offline_created_meshlets_); // run one mesh shader instance per meshlet
			}
			else
			 
			{ 
				opengl_context_->loaded_api_extensions.glDrawMeshTasksNV(0, (num_offline_created_meshlets_ + 31) / 32); // run in task shader groups of 32 primitives
			}
		}
	}
}
