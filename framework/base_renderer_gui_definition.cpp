#include "base_renderer.hpp"

#include "CPU_UBO_types.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <implot/implot.h>

#define IMVIEWGUIZMO_IMPLEMENTATION
#include "ImViewGuizmo.h"

#include <glm/gtx/string_cast.hpp>

#include <cassert>
#include <numeric>
#include <stack>

namespace framework {
	
#define YAW_ARRAY_IDX (0)
#define PITCH_ARRAY_IDX (1)
#define ROLL_ARRAY_IDX (2)

	void BaseRenderer::start_gui_frame() {
		if (NO_GUI) {
			return;
		}

		auto *renderer_ = this;

		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ 500.0f, 1000.0f });

		std::string const main_window_title = "window_title_base_ MODE Placeholder";


		ImGui::Begin(main_window_title.c_str());

		if (ImGui::TreeNodeEx("General Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Show Model Config Window", &renderer_->model_conf.show_window);
			ImGui::Checkbox("Show Camera Config Window", &renderer_->camera_conf.show_window);
			ImGui::Checkbox("Render Isosurface", &renderer_->renderer_conf.general.render_isosurface);
			ImGui::Checkbox("Render Auxiliaries", &renderer_->renderer_conf.general.render_auxiliaries);
			ImGui::TreePop();
		}


		ImGui::Separator();
		ImGui::Separator();
		if (ImGui::TreeNodeEx("Per App Start Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
			auto const& per_app_start_statistics = renderer_->get_per_app_start_performance_statistics();
			ImGui::Text("Min Max Volume Computation time: %.2f ms", per_app_start_statistics.min_max_computation_time_ms);
			ImGui::TreePop();
		}
		ImGui::Separator();
		if (ImGui::TreeNodeEx("Per Frame Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {

			{
				int32_t renderer_timer_mode_id = static_cast<int32_t>(renderer_->get_gpu_timer_mode());

				if (ImGui::RadioButton("Global Timing", &renderer_timer_mode_id, static_cast<int32_t>(framework::timer_mode::GLOBAL_FRAME))) {
				}
				if (ImGui::RadioButton("All Frame Components", &renderer_timer_mode_id, static_cast<int32_t>(framework::timer_mode::PER_PIPELINE_COMPONENT))) {
				}

				renderer_->set_gpu_timer_mode(static_cast<framework::timer_mode>(renderer_timer_mode_id));
			}

			auto const per_frame_statistics = renderer_->get_per_frame_statistics();
			{
				switch (per_frame_statistics.renderer_timer_mode) {
					case framework::timer_mode::GLOBAL_FRAME: {
						ImGui::Text("Total elapsed seconds: %.5f ms", renderer_->get_per_frame_statistics().measured_time.global_time_ms);
						break;
					}
					case framework::timer_mode::PER_PIPELINE_COMPONENT: {
						ImGui::Text("Occupancy Filtering time: %.5f ms", renderer_->get_per_frame_statistics().measured_time.per_component.dense_occupancy_computation_time_ms);
						ImGui::Text("Extraction time: %.5f ms", renderer_->get_per_frame_statistics().measured_time.per_component.pure_extraction_time_ms);
						ImGui::Text("Combined Extraction time: %.5f ms", renderer_->get_per_frame_statistics().measured_time.per_component.dense_occupancy_computation_time_ms + renderer_->get_per_frame_statistics().measured_time.per_component.pure_extraction_time_ms);
						ImGui::Text("Isosurface draw time: %.5f ms", renderer_->get_per_frame_statistics().measured_time.per_component.pure_isosurface_rendering_time_ms);
						ImGui::Text("Aux Vis draw time: %.5f ms", renderer_->get_per_frame_statistics().measured_time.per_component.pure_auxiliary_vis_rendering_time_ms);
						ImGui::Text("--------------------------------");
						ImGui::Text("Sum of components: %.5f ms", renderer_->get_per_frame_statistics().measured_time.per_component.accumulated_frame_time_ms);
						break;
					}
				}
			}

			ImGui::NewLine();

			{
				int32_t renderer_memory_footprint_mode_id = static_cast<int32_t>(renderer_->get_gpu_memory_footprint_mode());

				if (ImGui::RadioButton("No Memory Footprint Measurement", &renderer_memory_footprint_mode_id, static_cast<int32_t>(framework::memory_footprint_mode::NONE))) {
				}
				if (ImGui::RadioButton("Global Memory Footprint", &renderer_memory_footprint_mode_id, static_cast<int32_t>(framework::memory_footprint_mode::GLOBAL_FRAME))) {
				}
				if (ImGui::RadioButton("All Memory Components", &renderer_memory_footprint_mode_id, static_cast<int32_t>(framework::memory_footprint_mode::PER_MEMORY_COMPONENT))) {
				}

				renderer_->set_gpu_memory_footprint_mode(static_cast<framework::memory_footprint_mode>(renderer_memory_footprint_mode_id));
			}

			
			{
				switch (per_frame_performance_stats_.renderer_memory_footprint_mode) {
					case framework::memory_footprint_mode::NONE: {
						ImGui::Text("Memory footprint is not recorded in NONE-mode");
						ImGui::Text("Memory footprint is not recorded in NONE-mode");
						break;
					}
					case framework::memory_footprint_mode::GLOBAL_FRAME: {
						ImGui::Text("Total Memory Footprint: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.total_amount_byte / (1024.0*1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.total_amount_byte);
						break;
					}
					case framework::memory_footprint_mode::PER_MEMORY_COMPONENT: {
						ImGui::Text("Persistent Descriptor Memory Footprint: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.block_descriptors_persistent_part / (1024.0 * 1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.block_descriptors_persistent_part);
						ImGui::Text("Live Descriptor Memory Footprint: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.block_descriptors_live_part / (1024.0 * 1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.block_descriptors_live_part);
						ImGui::Text("Meshlet Descriptor Memory Footprint: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.meshlet_descriptors / (1024.0 * 1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.meshlet_descriptors);
						ImGui::Text("Culling Info Memory Footprint: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.culling_information / (1024.0 * 1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.culling_information);
						ImGui::Text("Vertex Position Memory Footprint: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.vertex_positions / (1024.0 * 1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.vertex_positions);
						ImGui::Text("Vertex Normals Memory Footprint: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.vertex_normals / (1024.0 * 1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.vertex_normals);
						ImGui::Text("Vertex Indices Memory Footprint: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.indices / (1024.0 * 1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.indices);
						ImGui::Text("--------------------------------");
						ImGui::Text("Sum of components: %ld MB (%ld Byte)", static_cast<int64_t>(renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.total / (1024.0 * 1024.0)), renderer_->get_per_frame_statistics().measured_memory_footprints.separated_memory_amounts_byte.total);
						break;
					}
				}

			}

			ImGui::TreePop();
		}
		ImGui::End();

		if (renderer_->model_conf.show_window) {
			ImGui::Begin("Model Config");

			{
				auto current_shading_base_color = renderer_->get_shading_base_color();
				if (ImGui::ColorEdit3("Shading Base Color", glm::value_ptr(current_shading_base_color))) {
					renderer_->set_shading_base_color(current_shading_base_color);
				}
			}

			ImGui::End();
		}

		if (renderer_->camera_conf.show_window) {
			ImGui::Begin("Camera Config");
			{
				uint32_t const available_number_of_cameras = renderer_->get_number_of_cameras();
				uint32_t const last_available_camera_idx = available_number_of_cameras - 1;

				{
					int32_t active_culling_reference_camera_idx = renderer_->get_active_culling_reference_camera_index();
					if (ImGui::SliderInt("Culling Reference Camera Idx: ", &active_culling_reference_camera_idx, 0, last_available_camera_idx)) {
						renderer_->set_active_culling_reference_camera_by_index(active_culling_reference_camera_idx);
					}
				}

				{
					int32_t active_point_of_view_camera_idx = renderer_->get_active_point_of_view_camera_index();
					if (ImGui::SliderInt("POV Camera Idx: ", &active_point_of_view_camera_idx, 0, last_available_camera_idx)) {
						renderer_->set_active_point_of_view_camera_by_index(active_point_of_view_camera_idx);
					}
				}

				ImGui::BeginChild("Culling Reference Camera Config");
				ImGui::Separator();
				ImGui::Separator();
				ImGui::Text("Culling Reference Camera");
				ImGui::Separator();
				ImGui::Text("Configurables");
				{
					auto const& culling_reference_camera = renderer_->get_current_culling_reference_camera();
					ImGui::SliderFloat3("Main Cam Pos Slider: ", glm::value_ptr(culling_reference_camera->public_descriptor.camera_position), -10.0f, 10.0f); ImGui::InputFloat3("Main Cam Pos Text: ", glm::value_ptr(culling_reference_camera->public_descriptor.camera_position));
				}
				{
					auto const& pov_camera = renderer_->get_current_point_of_view_camera();
					ImGui::SliderFloat3("POV Cam Pos Slider: ", glm::value_ptr(pov_camera->public_descriptor.camera_position), -10.0f, 10.0f); ImGui::InputFloat3("POV Cam Pos Text: ", glm::value_ptr(pov_camera->public_descriptor.camera_position));
				}
				ImGui::Separator();
				ImGui::Text("Culling Reference Camera Monitor");

				ImGui::Indent();
				if (ImGui::TreeNodeEx("Culling Reference Camera Mat Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto const culling_reference_camera_cpu_buffer = renderer_->get_culling_reference_camera_cpu_buffer();
					ImGui::Indent();
					ImGui::Text((glm::to_string(culling_reference_camera_cpu_buffer.camera_mat)).c_str());
					ImGui::Unindent();
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Culling Reference View Mat Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto const culling_reference_camera_cpu_buffer = renderer_->get_culling_reference_camera_cpu_buffer();
					ImGui::Indent();
					ImGui::Text((glm::to_string(culling_reference_camera_cpu_buffer.view_mat)).c_str());
					ImGui::Unindent();
					ImGui::TreePop();
				}

				ImGui::Unindent();
				ImGui::Separator();

				ImGui::Indent();
				ImGui::Text("Combined Model Culling Reference Camera Monitor");

				if (ImGui::TreeNodeEx("Culling Reference MVP Mat Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::Indent();
					auto const main_combined_model_culling_reference_camera_cpu_buffer = renderer_->get_combined_model_culling_reference_camera_cpu_buffer();

					ImGui::Text(("Culling Reference MVP Mat: " + glm::to_string(main_combined_model_culling_reference_camera_cpu_buffer.model_view_projection_mat)).c_str());
					ImGui::Unindent();
					ImGui::TreePop();
				}

				ImGui::Unindent();

				ImGui::Separator();
				ImGui::Separator();

				ImGui::Separator();
				ImGui::Text("POV Camera Monitor");

				ImGui::Indent();
				if (ImGui::TreeNodeEx("POV Camera Mat Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto const point_of_view_camera_cpu_buffer = renderer_->get_point_of_view_camera_cpu_buffer();
					ImGui::Text((glm::to_string(point_of_view_camera_cpu_buffer.camera_mat)).c_str());
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("POV View Mat Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto const point_of_view_camera_cpu_buffer = renderer_->get_point_of_view_camera_cpu_buffer();
					ImGui::Text((glm::to_string(point_of_view_camera_cpu_buffer.view_mat)).c_str());
					ImGui::TreePop();
				}
				ImGui::Unindent();
				ImGui::Separator();


				ImGui::Indent();
				ImGui::Text("Combined Model POV Camera Monitor");

				if (ImGui::TreeNodeEx("POV MVP Mat Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::Indent();
					auto const main_combined_model_point_of_view_camera_cpu_buffer = renderer_->get_combined_model_point_of_view_camera_cpu_buffer();
					ImGui::Text(("MVP Mat: " + glm::to_string(main_combined_model_point_of_view_camera_cpu_buffer.model_view_projection_mat)).c_str());
					ImGui::Unindent();
					ImGui::TreePop();
				}

				ImGui::Unindent();

				ImGui::Separator();
				ImGui::Separator();

				ImGui::EndChild();
			}
			ImGui::End();
		}

		if (renderer_->renderer_conf.general.render_isosurface) {
			ImGui::Begin("Persistent Extraction Compute Mode");

			ImGui::Separator();
			ImGui::Separator();
			ImGui::Text("Extraction Path:");

			bool changed_a_compute_parameter = false;

			int32_t post_persistent_extraction_compute_mode_index = static_cast<int32_t>(renderer_->renderer_conf.indirect_rendering_compute.compute_mode);
			int32_t const pre_gui_interaction_extraction_compute_mode_index = post_persistent_extraction_compute_mode_index;

			if (ImGui::RadioButton("Compute Shader-Path", &post_persistent_extraction_compute_mode_index, static_cast<int32_t>(framework::IndirectRenderingComputeMode::COMPUTE_SHADER_BASED_EXTRACTION))) {
			}
			if (ImGui::RadioButton("Task-Mesh Shader-Path", &post_persistent_extraction_compute_mode_index, static_cast<int32_t>(framework::IndirectRenderingComputeMode::TASK_MESH_SHADER_BASED_EXTRACTION))) {
			}
			if (ImGui::RadioButton("Meshopt Offline Meshlet Creation", &post_persistent_extraction_compute_mode_index, static_cast<int32_t>(framework::IndirectRenderingComputeMode::MESHOPT_OFFLINE_MESHLET_COMPUTATION))) {
			}

			if (post_persistent_extraction_compute_mode_index != pre_gui_interaction_extraction_compute_mode_index) {
				renderer_->renderer_conf.indirect_rendering_compute.compute_mode = static_cast<framework::IndirectRenderingComputeMode>(post_persistent_extraction_compute_mode_index);
				changed_a_compute_parameter = true;
			}
			ImGui::Separator();


			if (framework::IndirectRenderingComputeMode::COMPUTE_SHADER_BASED_EXTRACTION == renderer_->renderer_conf.indirect_rendering_compute.compute_mode) {
				ImGui::Text("Compute-Style Extraction Mode:");

				int32_t post_compute_style_extraction_mode_index = static_cast<int32_t>(renderer_->renderer_conf.indirect_rendering_compute.compute_shader_path_extraction_mode);
				int32_t const pre_compute_style_extraction_mode_index = post_compute_style_extraction_mode_index;

				if (ImGui::RadioButton("Single Indexed Arrays", &post_compute_style_extraction_mode_index, static_cast<int32_t>(framework::ComputeShaderExtractionMode::SINGLE_INDEX_ARRAY))) {}
				if (ImGui::RadioButton("Blockwise 32 bit", &post_compute_style_extraction_mode_index, static_cast<int32_t>(framework::ComputeShaderExtractionMode::BLOCKWISE_32BIT_INDICES))) {}
				if (ImGui::RadioButton("Blockwise 16 bit", &post_compute_style_extraction_mode_index, static_cast<int32_t>(framework::ComputeShaderExtractionMode::BLOCKWISE_16BIT_INDICES))) {}

				if (post_compute_style_extraction_mode_index != pre_compute_style_extraction_mode_index) {
					renderer_->renderer_conf.indirect_rendering_compute.compute_shader_path_extraction_mode = static_cast<framework::ComputeShaderExtractionMode>(post_compute_style_extraction_mode_index);
					changed_a_compute_parameter = true;
				}
			}

			if (changed_a_compute_parameter) {
				renderer_->force_reextration_for_next_frame();
			}
			ImGui::End();

			ImGui::Begin("Persistent Extraction Draw Mode");

			if (!(framework::IndirectRenderingComputeMode::COMPUTE_SHADER_BASED_EXTRACTION == renderer_->renderer_conf.indirect_rendering_compute.compute_mode)
				) {
				bool use_task_and_mesh_shader_combination = (renderer_->renderer_conf.isosurface.meshlet_forward_rendering_pipeline_mode == framework::MeshletForwardRenderPipelineMode::TASK_AND_MESH_SHADER_ONLINE_CULLING);

				bool toggle_task_and_mesh_shader_culling_modes = false;

				ImGui::Columns(2, NULL, false);

				ImGui::TextUnformatted("Task+Mesh Shader");
				if (ImGui::Checkbox("Use Task Shader for Online Culling", &use_task_and_mesh_shader_combination)) {
					toggle_task_and_mesh_shader_culling_modes = true;
				};

				ImGui::NextColumn();
				ImGui::Text("Max num occupied PMB cells:");
				ImGui::Text("===========================");
				ImGui::RadioButton(" 32 Occ. Cells", &max_num_occupied_blocks_for_region_analysis_kernels_, 32);
				ImGui::RadioButton(" 64 Occ. Cells", &max_num_occupied_blocks_for_region_analysis_kernels_, 64);
				ImGui::RadioButton(" 96 Occ. Cells", &max_num_occupied_blocks_for_region_analysis_kernels_, 96);
				ImGui::RadioButton("128 Occ. Cells", &max_num_occupied_blocks_for_region_analysis_kernels_, 128);

				ImGui::Columns(1);

				if (toggle_task_and_mesh_shader_culling_modes) {
					renderer_->renderer_conf.isosurface.meshlet_forward_rendering_pipeline_mode = static_cast<framework::MeshletForwardRenderPipelineMode>((static_cast<int32_t>(renderer_->renderer_conf.isosurface.meshlet_forward_rendering_pipeline_mode) + 1) % 2);
					renderer_->force_reextration_for_next_frame();
				}
			}

			ImGui::End();
		}

		if (renderer_->renderer_conf.general.render_auxiliaries) {
			ImGui::Begin("Auxiliary Visualization Settings");
			ImGui::Checkbox("Draw Coordinate System Axes", &renderer_->renderer_conf.aux_vis.render_coordinate_axes);
			ImGui::Checkbox("Draw Culling Reference Camera Frustum", &renderer_->renderer_conf.aux_vis.render_culling_reference_camera_frustum);
			ImGui::Checkbox("Draw Volume Boundaries", &renderer_->renderer_conf.aux_vis.render_volume_boundaries);
			ImGui::End();
		}
	}

	void BaseRenderer::end_gui_frame() {
		if (NO_GUI) {
			return;
		}
	}
}