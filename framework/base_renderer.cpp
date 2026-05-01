#include "base_renderer.hpp"

#include "CPU_UBO_types.hpp"

// marching cubes tables as seen in CUDA samples repo
#include <mc_tables_nvidia/tables.h>
//tables that contain information about unique vertices per cell, derived from the nv tables, such that
//one efficiently actually only creates a maximum of 12 vertices per cell, instead of 15 (which contain duplicates)

#include  "unified_derived_mc_tables.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <implot/implot.h>

#include <glm/gtx/string_cast.hpp>

#include <omp.h>

#include <cassert>
#include <numeric>
#include <stack>


namespace framework {
	
	bool  BaseRenderer::NO_GUI = false;

	void BaseRenderer::set_NO_GUI(bool value)
	{
		NO_GUI = value;

	}

#define YAW_ARRAY_IDX (0)
#define PITCH_ARRAY_IDX (1)
#define ROLL_ARRAY_IDX (2)

	BaseRenderer::BaseRenderer(int32_t preconfigured_index_buffer_size_in_MB, int32_t preconfigured_position_buffer_size_in_MB, int32_t preconfigured_normal_buffer_size_in_MB) :
		preconfigured_index_buffer_size_in_MB_{ preconfigured_index_buffer_size_in_MB}, 
		preconfigured_position_buffer_size_in_MB_{ preconfigured_position_buffer_size_in_MB },
		preconfigured_normal_buffer_size_in_MB_{ preconfigured_normal_buffer_size_in_MB } {

	}

	void BaseRenderer::initialize(/*GLFWwindow* window_, */glm::ivec2 const& requested_window_resolution, glm::vec3 culling_camera_reference_position, glm::vec3 pov_camera_reference_position) {
		window_resolution_ = requested_window_resolution;

		initialize_context_(/**/requested_window_resolution);

		initialize_final_frame_buffers_();
		initialize_default_culling_reference_camera_(culling_camera_reference_position);
		initialize_additional_cameras_(pov_camera_reference_position);

		//create_shader_programs_();
	}

	void BaseRenderer::create_shader_programs() {
		create_shader_programs_();
	}


	void BaseRenderer::tear_down() {
	}

	void BaseRenderer::precompile_shaders() {
		precompile_shaders_ = true;
	}

	bool BaseRenderer::is_running() const {
		return !glfwWindowShouldClose(base_context_->window_);
	}

	void BaseRenderer::build_meshlets_offline(uint32_t& in_out_vertex_count, uint32_t& in_out_index_count) {
		meshopt_Stream streams[] = {
			{positions_cpu_.data(), sizeof(glm::vec3), sizeof(glm::vec3)},
			{normals_cpu_.data() , sizeof(glm::vec3), sizeof(glm::vec3)}
		};

		index_remap_table_.resize(in_out_vertex_count);
		size_t deduplicated_vertex_count = meshopt_generateVertexRemapMulti(index_remap_table_.data(), indices_cpu_.data(), in_out_index_count, in_out_vertex_count, &streams[0], sizeof(streams) / sizeof(streams[0]));

		remapped_positions_.resize(deduplicated_vertex_count);
		remapped_normals_.resize(deduplicated_vertex_count);

		meshopt_remapVertexBuffer(remapped_positions_.data(), positions_cpu_.data(), in_out_vertex_count, sizeof(glm::vec3), index_remap_table_.data());
		meshopt_remapVertexBuffer(remapped_normals_.data(), normals_cpu_.data(), in_out_vertex_count, sizeof(glm::vec3), index_remap_table_.data());
		remapped_indices_.resize(in_out_index_count);
		meshopt_remapIndexBuffer(remapped_indices_.data(), indices_cpu_.data(), in_out_index_count, index_remap_table_.data());

		size_t max_meshlets = meshopt_buildMeshletsBound(in_out_index_count, MAX_OFFLINE_CREATED_VERTICES_PER_MESHLET, MAX_OFFLINE_CREATED_TRIANGLES_PER_MESHLET);

		auto& meshlets = built_meshlets_cpu_;
		meshlets.resize(max_meshlets);

		meshlet_vertex_indices_ = std::vector<unsigned int>(in_out_index_count);
		meshlet_triangle_indices_ = std::vector<unsigned char>(in_out_index_count);



		size_t actual_meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertex_indices_.data(), meshlet_triangle_indices_.data(), remapped_indices_.data(),
			in_out_index_count, (float*)&remapped_positions_[0], in_out_index_count, sizeof(glm::vec3), MAX_OFFLINE_CREATED_VERTICES_PER_MESHLET, MAX_OFFLINE_CREATED_TRIANGLES_PER_MESHLET, 1.0f);
		//meshopt_generatePositionRemap

		meshlets.resize(actual_meshlet_count);

		meshlet_bounding_spheres_.resize(actual_meshlet_count);


		#pragma omp parallel for
		for (int32_t meshlet_index = 0; meshlet_index < actual_meshlet_count; ++meshlet_index) {
			auto const& m = meshlets[meshlet_index];
			meshopt_optimizeMeshlet((meshlet_vertex_indices_.data() + m.vertex_offset), meshlet_triangle_indices_.data() + m.triangle_offset, m.triangle_count, m.vertex_count);

			::meshopt_Bounds const b = meshopt_computeMeshletBounds(
				&meshlet_vertex_indices_[m.vertex_offset], &meshlet_triangle_indices_[m.triangle_offset], m.triangle_count,
				(float*)(&remapped_positions_[0]), in_out_vertex_count, sizeof(float) * 3);
			meshlet_bounding_spheres_[meshlet_index] = glm::vec4(b.center[0], b.center[1], b.center[2], b.radius);
		}


		const meshopt_Meshlet& last = meshlets[actual_meshlet_count - 1];


		meshlet_vertex_indices_.resize(last.vertex_offset + last.vertex_count);
		meshlet_triangle_indices_.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
		

		total_num_extracted_meshlets_ = meshlets.size();
		num_offline_created_meshlets_ = actual_meshlet_count;

		in_out_vertex_count = deduplicated_vertex_count;
	}

	void BaseRenderer::set_use_NV_mesh_shader_extension(bool use_NV_mesh_shader_extension) {
		use_NV_mesh_shader_extension_ = use_NV_mesh_shader_extension;
	}
	void BaseRenderer::set_max_num_occupied_blocks_for_region_analysis_kernels(int32_t max_num_occupied_blocks_for_region_analysis_kernels_to_set) {
		max_num_occupied_blocks_for_region_analysis_kernels_ = max_num_occupied_blocks_for_region_analysis_kernels_to_set;
	}


	void BaseRenderer::register_window_callbacks() {
		glfwMakeContextCurrent(base_context_->window_);
		glfwSetWindowUserPointer(base_context_->window_, this);
		
		auto key_callback_func_wrapper = [](GLFWwindow* window, int key, int scancode, int action, int mod)

			{
				auto* base_renderer = static_cast<BaseRenderer*>(glfwGetWindowUserPointer(window));
				if (base_renderer->base_context_->is_in_OpenGL_mode_()) {
					ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mod);
				}
				base_renderer->key_callback_(window, key, scancode, action, mod);
			};
		
		base_context_->register_key_callback_func(key_callback_func_wrapper);


		auto scroll_callback_func_wrapper = [](GLFWwindow* window, double x_offset, double y_offset)

			{
				auto* base_renderer = static_cast<BaseRenderer*>(glfwGetWindowUserPointer(window));
				if (base_renderer->base_context_->is_in_OpenGL_mode_()) {
					ImGui_ImplGlfw_ScrollCallback(window, x_offset, y_offset);
				}
				base_renderer->scroll_callback_(window, x_offset, y_offset);
			};

		base_context_->register_scroll_callback_func(scroll_callback_func_wrapper);


		auto mouse_moved_callback_func_wrapper = [](GLFWwindow* window, double mouse_pos_x, double mouse_pos_y)

			{
				auto* base_renderer = static_cast<BaseRenderer*>(glfwGetWindowUserPointer(window));
				if (base_renderer->base_context_->is_in_OpenGL_mode_()) {
					ImGui_ImplGlfw_CursorPosCallback(window, mouse_pos_x, mouse_pos_y);
				}
				base_renderer->mouse_moved_callback_(window, mouse_pos_x, mouse_pos_y);
			};

		base_context_->register_mouse_moved_callback_func(mouse_moved_callback_func_wrapper);

		auto mouse_clicked_callback_func_wrapper = [](GLFWwindow* window, int button, int action, int mods)

			{
				auto* base_renderer = static_cast<BaseRenderer*>(glfwGetWindowUserPointer(window));
				if (base_renderer->base_context_->is_in_OpenGL_mode_()) {
					ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
				}
				base_renderer->mouse_click_callback_(window, button, action, mods);
			};

		base_context_->register_mouse_clicked_callback_func(mouse_clicked_callback_func_wrapper);

	}

	void BaseRenderer::initialize_default_culling_reference_camera_(glm::vec3 culling_camera_reference_position) {
		default_culling_reference_camera_parameters_ = create_default_camera_descriptor_(window_resolution_, culling_camera_reference_position);

		culling_reference_camera_ = std::make_shared<BaseCamera>(default_culling_reference_camera_parameters_);

		add_existing_camera_to_database_(culling_reference_camera_);
	}

	void BaseRenderer::initialize_additional_cameras_(glm::vec3 additional_camera_reference_position) {
		auto additional_camera_parameters = create_default_camera_descriptor_(window_resolution_, additional_camera_reference_position);

		auto additional_camera_to_add = std::make_shared<BaseCamera>(additional_camera_parameters);

		add_existing_camera_to_database_(additional_camera_to_add);
	}

	void BaseRenderer::assign_culling_reference_camera_(std::shared_ptr<BaseCamera> const& camera_to_become_main_cam) {
		culling_reference_camera_ = camera_to_become_main_cam;
	}

	void BaseRenderer::assign_point_of_view_camera_(std::shared_ptr<BaseCamera> const& camera_to_become_point_of_view_cam) {
		point_of_view_camera_ = camera_to_become_point_of_view_cam;
	}

	void BaseRenderer::add_existing_camera_to_database_(std::shared_ptr<BaseCamera> const& new_camera) {
		cameras_.push_back(new_camera);
	
		if (nullptr == point_of_view_camera_) {
			assign_point_of_view_camera_(new_camera);
		}
	}

	void BaseRenderer::scroll_callback_(GLFWwindow* window, double x_offset, double y_offset) {
		
		if (!NO_GUI) {
			if (draw_gui_ && ImGui::GetIO().WantCaptureMouse) {
				return;
			}
		}
		
		std::shared_ptr<framework::BaseCamera> current_pov_camera = get_current_point_of_view_camera();

		float default_delta = 0.025f;

		if (y_offset > 0.0f) {
			increase_isovalue(default_delta);
		}
		else if (y_offset < 0.0f) {
			decrease_isovalue(default_delta);
		}

	}

	void BaseRenderer::reserve_space_for_number_of_volume_datasets_(uint32_t number_of_different_volume_datasets) {
		volumes_.reserve(number_of_different_volume_datasets);
	}

	void BaseRenderer::add_volume_dataset_(std::shared_ptr<BaseVolume> const& volume_dataset) {
		volumes_.push_back(volume_dataset);
		yaw_pitch_roll_angles_in_degree_.emplace_back(0.0f);
		if (nullptr == main_volume_descriptor_) {
			assert(nullptr == main_volume_descriptor_);
			main_volume_descriptor_ = std::make_unique<VolumeDescriptor>(volume_dataset->get_descriptor());
			assert(nullptr != main_volume_descriptor_);
		}
	}

	void BaseRenderer::update_total_elapsed_renderer_time_() {
		std::chrono::time_point<std::chrono::system_clock> current_timestamp = std::chrono::system_clock::now();
		auto elapsed_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(current_timestamp - latest_timestamp_).count();
		latest_timestamp_ = current_timestamp;
		double const elapsed_seconds_as_double = (elapsed_microseconds / 1e6);
		total_elapsed_seconds_ += elapsed_seconds_as_double;

		if (auto_rotate_) {
			total_elapsed_seconds_for_auto_rotation_ += elapsed_seconds_as_double;
		}
	}

	cpu_culling_reference_camera_buffer BaseRenderer::get_culling_reference_camera_cpu_buffer() const {
		if (nullptr != cpu_culling_reference_camera_buffer_) {
			return *cpu_culling_reference_camera_buffer_;
		}
		
			return {};
	
	}

	cpu_combined_model_culling_reference_camera_buffer BaseRenderer::get_combined_model_culling_reference_camera_cpu_buffer() const {
		if (nullptr != cpu_combined_model_culling_reference_camera_buffer_) {
			return *cpu_combined_model_culling_reference_camera_buffer_;
		}
		
			return {};
	
	}

	cpu_point_of_view_camera_buffer BaseRenderer::get_point_of_view_camera_cpu_buffer() const {
		if (nullptr != cpu_point_of_view_camera_buffer_) {
			return *cpu_point_of_view_camera_buffer_;
		}
		
			return {};
	
	}

	cpu_combined_model_point_of_view_camera_buffer BaseRenderer::get_combined_model_point_of_view_camera_cpu_buffer() const {
		if (nullptr != cpu_combined_model_point_of_view_camera_buffer_) {
			return *cpu_combined_model_point_of_view_camera_buffer_;
		}
		
			return {};
	
	}

	void BaseRenderer::initialize_culling_reference_camera_buffer_cpu_memory_(cpu_culling_reference_camera_buffer& cpu_memory_culling_reference_camera_buffer_to_initialize) {
		assert(nullptr == cpu_culling_reference_camera_buffer_);

		cpu_culling_reference_camera_buffer_ = &cpu_memory_culling_reference_camera_buffer_to_initialize;
		// tie CPU memory to base renderer for direct access

		update_culling_reference_camera_buffer_cpu_memory_();


		//third: flag matrix as dirty
		cpu_culling_reference_camera_buffer_->is_dirty = true;

	}

	void BaseRenderer::update_culling_reference_camera_buffer_cpu_memory_() {
		auto const culling_reference_camera_buffer_pre_update = *cpu_culling_reference_camera_buffer_;

		culling_reference_camera_->apply_navigation_input_since_last_frame();
		culling_reference_camera_->update_view_buffer_cpu_data_();

		cpu_culling_reference_camera_buffer_->camera_mat = culling_reference_camera_->internal_descriptor_.extrinsics_.camera_mat;
		cpu_culling_reference_camera_buffer_->view_mat = culling_reference_camera_->internal_descriptor_.extrinsics_.view_mat;
		cpu_culling_reference_camera_buffer_->projection_mat = culling_reference_camera_->internal_descriptor_.intrinsics_.projection_mat;
		cpu_culling_reference_camera_buffer_->inverse_projection_mat = culling_reference_camera_->internal_descriptor_.intrinsics_.inverse_projection_mat;
		cpu_culling_reference_camera_buffer_->view_projection_mat = cpu_culling_reference_camera_buffer_->projection_mat * cpu_culling_reference_camera_buffer_->view_mat;
		cpu_culling_reference_camera_buffer_->inverse_view_projection_mat = glm::inverse(cpu_culling_reference_camera_buffer_->view_projection_mat);
		cpu_culling_reference_camera_buffer_->pos_world_space = cpu_culling_reference_camera_buffer_->camera_mat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		cpu_culling_reference_camera_buffer_->frustum_planes = culling_reference_camera_->internal_descriptor_.intrinsics_.frustum_planes;
		cpu_culling_reference_camera_buffer_->near_far_padding_x2 = culling_reference_camera_->internal_descriptor_.intrinsics_.near_far_padding_x2;

		if (culling_reference_camera_buffer_pre_update != *cpu_culling_reference_camera_buffer_) {
			cpu_culling_reference_camera_buffer_->is_dirty = true;
		}
	}


	void BaseRenderer::initialize_point_of_view_camera_buffer_cpu_memory_(cpu_point_of_view_camera_buffer& cpu_memory_point_of_view_buffer_to_initialize) {
		assert(nullptr == cpu_point_of_view_camera_buffer_);

		cpu_point_of_view_camera_buffer_ = &cpu_memory_point_of_view_buffer_to_initialize;
		// tie CPU memory to base renderer for direct access

		
		update_point_of_view_camera_buffer_cpu_memory_();

		//third: flag matrix as dirty
		cpu_point_of_view_camera_buffer_->is_dirty = true;

	}

	void BaseRenderer::update_point_of_view_camera_buffer_cpu_memory_() {
		if (point_of_view_camera_ == nullptr || cpu_point_of_view_camera_buffer_ == nullptr) {
			return;
		}

		auto const point_of_view_camera_buffer_pre_update = *cpu_point_of_view_camera_buffer_;

		point_of_view_camera_->update_view_buffer_cpu_data_();

		cpu_point_of_view_camera_buffer_->camera_mat = point_of_view_camera_->internal_descriptor_.extrinsics_.camera_mat;
		cpu_point_of_view_camera_buffer_->view_mat = point_of_view_camera_->internal_descriptor_.extrinsics_.view_mat;
		cpu_point_of_view_camera_buffer_->projection_mat = point_of_view_camera_->internal_descriptor_.intrinsics_.projection_mat;
		cpu_point_of_view_camera_buffer_->inverse_projection_mat = point_of_view_camera_->internal_descriptor_.intrinsics_.inverse_projection_mat;
		cpu_point_of_view_camera_buffer_->view_projection_mat = cpu_point_of_view_camera_buffer_->projection_mat * cpu_point_of_view_camera_buffer_->view_mat;
		cpu_point_of_view_camera_buffer_->inverse_view_projection_mat = glm::inverse(cpu_point_of_view_camera_buffer_->view_projection_mat);
		cpu_point_of_view_camera_buffer_->pos_world_space = cpu_point_of_view_camera_buffer_->camera_mat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		cpu_point_of_view_camera_buffer_->frustum_planes = point_of_view_camera_->internal_descriptor_.intrinsics_.frustum_planes;
		cpu_point_of_view_camera_buffer_->near_far_padding_x2 = point_of_view_camera_->internal_descriptor_.intrinsics_.near_far_padding_x2;

		if (point_of_view_camera_buffer_pre_update != *cpu_point_of_view_camera_buffer_) {
			cpu_point_of_view_camera_buffer_->is_dirty = true;
		}
	}


	void BaseRenderer::initialize_model_buffer_cpu_memory_(cpu_model_buffer& cpu_memory_model_buffer_to_initialize) {
		assert(nullptr == cpu_model_buffer_);

		// tie CPU memory to base renderer for direct access from base class
		cpu_model_buffer_ = &cpu_memory_model_buffer_to_initialize;

		update_model_buffer_cpu_memory_();

		cpu_model_buffer_->is_dirty = true;
	}

	void BaseRenderer::update_model_buffer_cpu_memory_() {
		if (nullptr == cpu_model_buffer_) {
			return;
		}
		auto const model_buffer_pre_update = *cpu_model_buffer_;

		// [0,0,0;1,1,1] -> [-1,-1,-1;1,1,1]
		auto scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
		auto trans_mat = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, -1.0f));


		auto const& current_yaw_pitch_roll_angles = yaw_pitch_roll_angles_in_degree_[0];

		auto yaw_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(current_yaw_pitch_roll_angles[YAW_ARRAY_IDX]), glm::vec3(0.0f, 1.0f, 0.0f));
		auto pitch_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(current_yaw_pitch_roll_angles[PITCH_ARRAY_IDX]), glm::vec3(1.0f, 0.0f, 0.0f));
		auto roll_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(current_yaw_pitch_roll_angles[ROLL_ARRAY_IDX]), glm::vec3(0.0f, 0.0f, -1.0f));
		

		point_of_view_camera_->apply_navigation_input_since_last_frame();
		auto user_input_based_transform = glm::inverse(point_of_view_camera_->get_rotation_matrix());

		static constexpr float AUTO_ROTATION_REVOLUTIONS_PER_SECOND = 0.3f;
		auto auto_rotation_matrix = glm::rotate(glm::mat4(1.0f),
			static_cast<float>(total_elapsed_seconds_for_auto_rotation_) * glm::pi<float>() * 2.0f * AUTO_ROTATION_REVOLUTIONS_PER_SECOND,
			glm::vec3(0.0f, 1.0f, 0.0f));

		cpu_model_buffer_->model_mat
			= auto_rotation_matrix
			* user_input_based_transform
			* roll_matrix * pitch_matrix * yaw_matrix
			* trans_mat * scale_mat;

		cpu_model_buffer_->model_mat[3][3] = 1.0f;
		cpu_model_buffer_->inv_model_mat = glm::inverse(cpu_model_buffer_->model_mat);


		//third: flag buffer as dirty in any case since it was initialized
		if (model_buffer_pre_update != *cpu_model_buffer_) {
			cpu_model_buffer_->is_dirty = true;
		}
	}

	void BaseRenderer::initialize_combined_model_culling_reference_camera_buffer_cpu_memory_(cpu_combined_model_culling_reference_camera_buffer& cpu_memory_combined_model_culling_reference_camera_buffer_to_initialize) {
		assert(nullptr == cpu_combined_model_culling_reference_camera_buffer_);

		cpu_combined_model_culling_reference_camera_buffer_ = &cpu_memory_combined_model_culling_reference_camera_buffer_to_initialize;
		// tie CPU memory to base renderer for direct access
		update_combined_model_culling_reference_camera_buffer_cpu_memory_();

		cpu_combined_model_culling_reference_camera_buffer_->is_dirty = true;
	}

	void BaseRenderer::update_combined_model_culling_reference_camera_buffer_cpu_memory_() {
		if (cpu_combined_model_culling_reference_camera_buffer_ == nullptr) {
			return;
		}
		auto const combined_model_culling_reference_camera_buffer_pre_update = *cpu_combined_model_culling_reference_camera_buffer_;

		cpu_combined_model_culling_reference_camera_buffer_->model_view_mat = cpu_culling_reference_camera_buffer_->view_mat * cpu_model_buffer_->model_mat;
		cpu_combined_model_culling_reference_camera_buffer_->inverse_model_view_mat = glm::inverse(cpu_combined_model_culling_reference_camera_buffer_->model_view_mat);
		cpu_combined_model_culling_reference_camera_buffer_->normal_mat = glm::transpose(cpu_combined_model_culling_reference_camera_buffer_->inverse_model_view_mat);
		cpu_combined_model_culling_reference_camera_buffer_->model_view_projection_mat = cpu_culling_reference_camera_buffer_->projection_mat * cpu_combined_model_culling_reference_camera_buffer_->model_view_mat;

		cpu_combined_model_culling_reference_camera_buffer_->vol_space_cam_pos = cpu_combined_model_culling_reference_camera_buffer_->inverse_model_view_mat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		if (combined_model_culling_reference_camera_buffer_pre_update != *cpu_combined_model_culling_reference_camera_buffer_) {
			cpu_combined_model_culling_reference_camera_buffer_->is_dirty = true;
		}
	}

	void BaseRenderer::initialize_combined_model_point_of_view_camera_buffer_cpu_memory_(cpu_combined_model_point_of_view_camera_buffer& cpu_memory_combined_model_point_of_view_camera_buffer_to_initialize) {
		assert(nullptr == cpu_combined_model_point_of_view_camera_buffer_);

		cpu_combined_model_point_of_view_camera_buffer_ = &cpu_memory_combined_model_point_of_view_camera_buffer_to_initialize;
		// tie CPU memory to base renderer for direct access
		update_combined_model_point_of_view_camera_buffer_cpu_memory_();

		cpu_combined_model_point_of_view_camera_buffer_->is_dirty = true;
	}

	void BaseRenderer::update_combined_model_point_of_view_camera_buffer_cpu_memory_() {
		if (cpu_combined_model_point_of_view_camera_buffer_ == nullptr) {
			return;
		}

		auto const combined_model_point_of_view_camera_buffer_pre_update = *cpu_combined_model_point_of_view_camera_buffer_;

		cpu_combined_model_point_of_view_camera_buffer_->model_view_mat = cpu_point_of_view_camera_buffer_->view_mat * cpu_model_buffer_->model_mat;
		cpu_combined_model_point_of_view_camera_buffer_->inverse_model_view_mat = glm::inverse(cpu_combined_model_point_of_view_camera_buffer_->model_view_mat);
		cpu_combined_model_point_of_view_camera_buffer_->normal_mat = glm::transpose(cpu_combined_model_point_of_view_camera_buffer_->inverse_model_view_mat);
		cpu_combined_model_point_of_view_camera_buffer_->model_view_projection_mat = cpu_point_of_view_camera_buffer_->projection_mat * cpu_combined_model_point_of_view_camera_buffer_->model_view_mat;

		cpu_combined_model_point_of_view_camera_buffer_->vol_space_cam_pos = cpu_combined_model_point_of_view_camera_buffer_->inverse_model_view_mat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		if (combined_model_point_of_view_camera_buffer_pre_update != *cpu_combined_model_point_of_view_camera_buffer_) {
			cpu_combined_model_point_of_view_camera_buffer_->is_dirty = true;
		}
	}



	void BaseRenderer::initialize_draw_parameters_buffer_(cpu_draw_parameters_buffer& cpu_memory_draw_parameters_buffer_to_initialize) {
		if (nullptr != cpu_draw_parameters_buffer_) {
			cpu_memory_draw_parameters_buffer_to_initialize = *cpu_draw_parameters_buffer_;
			delete cpu_draw_parameters_buffer_;
		}

		cpu_draw_parameters_buffer_ = &cpu_memory_draw_parameters_buffer_to_initialize;
	}

	void BaseRenderer::update_draw_parameters_buffer_cpu_memory_() {
		auto const draw_parameters_buffer_pre_update = *cpu_draw_parameters_buffer_;

		if (draw_parameters_buffer_pre_update != *cpu_draw_parameters_buffer_) {
			cpu_draw_parameters_buffer_->is_dirty = true;
		}
	}

	void BaseRenderer::initialize_extraction_parameters_buffer_(cpu_extraction_parameters_buffer& cpu_memory_extraction_parameters_buffer_to_initialize) {
		if (nullptr != cpu_extraction_parameters_buffer_) {
			cpu_memory_extraction_parameters_buffer_to_initialize = *cpu_extraction_parameters_buffer_;
			delete cpu_extraction_parameters_buffer_;
		}
		assert(!volumes_.empty());

		cpu_extraction_parameters_buffer_ = &cpu_memory_extraction_parameters_buffer_to_initialize;


		auto const& reference_volume = volumes_.back();
		glm::vec3 const reference_volume_resolution = static_cast<glm::vec3>(reference_volume->get_resolution());
		//cpu_extraction_parameters_buffer_->voxel_size = glm::vec4(glm::vec3(1.0f) / glm::vec3(descriptor_.resolution), 0.0f);
		//ubo at binding point 3
		cpu_extraction_parameters_buffer_->voxel_size = glm::vec4(glm::vec3(1.0f) / glm::vec3(reference_volume_resolution), 0.0f);
		cpu_extraction_parameters_buffer_->volume_resolution = glm::ivec4(reference_volume_resolution, 0);
	}

	void BaseRenderer::update_extraction_parameters_buffer_cpu_memory_() {
		auto const extraction_parameters_buffer_pre_update = *cpu_extraction_parameters_buffer_;

		if (extraction_parameters_buffer_pre_update != *cpu_extraction_parameters_buffer_) {
			cpu_extraction_parameters_buffer_->is_dirty = true;
		}
	}

	void BaseRenderer::initialize_render_frame_descriptor_buffer_(cpu_render_frame_descriptor_buffer& cpu_memory_render_frame_descriptor_buffer_to_initialize) {
		assert(nullptr == cpu_render_frame_descriptor_buffer_);

		cpu_render_frame_descriptor_buffer_ = &cpu_memory_render_frame_descriptor_buffer_to_initialize;
	}

	void BaseRenderer::update_render_frame_descriptor_buffer_cpu_memory_() {
		++cpu_render_frame_descriptor_buffer_->counter;
		cpu_render_frame_descriptor_buffer_->is_dirty = true;
	}



	void BaseRenderer::precompute_auxiliary_structures_() {
		per_app_start_performance_stats_.clear_all();

		if (nullptr == per_application_timers__min_max_volume_computation_timer_) {
			per_application_timers__min_max_volume_computation_timer_ = create_new_gpu_timer_instance_("min_max_volume_computation");
		}

		per_application_timers__min_max_volume_computation_timer_->start();
		compute_min_max_block_texture_();
		per_application_timers__min_max_volume_computation_timer_->end_wait_and_evaluate();
		
		per_app_start_performance_stats_.min_max_computation_time_ms = per_application_timers__min_max_volume_computation_timer_->get_elapsed_milliseconds();
	}


	void BaseRenderer::initialize_API_specific_graphics_objects_() {
		if (false == are_UBOs_allocated_) {
			initialize_UBOs_();
			are_UBOs_allocated_ = true;
		}

		//needs to happen after all shaders were created because they are used for parallel computation
		if (false == are_auxiliary_structures_allocated_) {
			precompute_auxiliary_structures_();
			are_auxiliary_structures_allocated_ = true;
		}
	}


	void BaseRenderer::operate_pre_frame() {
		update_total_elapsed_renderer_time_();

		if (false == are_API_specific_graphics_objects_initialized) {
			initialize_API_specific_graphics_objects_();

			are_API_specific_graphics_objects_initialized = true;
		}
		
		update_UBOs_();



		if (timer_mode::GLOBAL_FRAME == per_frame_performance_stats_.renderer_timer_mode) {
			if (nullptr == complete_frame_without_gui_rendering_timer_) {
				per_frame_performance_stats_.clear_all();
				complete_frame_without_gui_rendering_timer_ = create_new_gpu_timer_instance_("complete_frame_without_gui_rendering");
			}
		}

		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			if (nullptr == per_component_timers__dense_occupancy_computation_timer_) {
				per_component_timers__dense_occupancy_computation_timer_ = create_new_gpu_timer_instance_("per_component__dense_occupancy_computation_times_isosurface");
			}

			if (nullptr == per_component_timers__extraction_times_isosurface_extraction_timer_) {
				per_component_timers__extraction_times_isosurface_extraction_timer_ = create_new_gpu_timer_instance_("per_component__extraction_times_isosurface");
			}

			if (nullptr == per_component_timers__draw_times_isosurface_draw_timer_) {
				per_component_timers__draw_times_isosurface_draw_timer_ = create_new_gpu_timer_instance_("per_component_times__isosurface_rendering");
			}
			if (nullptr == per_component_timers__draw_times_aux_vis_draw_timer_) {
				per_component_timers__draw_times_aux_vis_draw_timer_ = create_new_gpu_timer_instance_("per_component_times__aux_vis_rendering");
			}
		}
	}

	int32_t BaseRenderer::get_mode_dependent_mesh_shader_extraction_shader_variant() const {
		int32_t num_occupied_PMB_cells_index_offset = 0;
		switch (max_num_occupied_blocks_for_region_analysis_kernels_) {
			case 32: {
				num_occupied_PMB_cells_index_offset = static_cast<int32_t>(framework::PersistentExtractionMode::MAX_OCCUPIED_PMB_CELLS_32);
				break;
			}
			case 64: {
				num_occupied_PMB_cells_index_offset = static_cast<int32_t>(framework::PersistentExtractionMode::MAX_OCCUPIED_PMB_CELLS_64);
				break;
			}
			case 96: {
				num_occupied_PMB_cells_index_offset = static_cast<int32_t>(framework::PersistentExtractionMode::MAX_OCCUPIED_PMB_CELLS_96);
				break;
			}
			case 128: {
				num_occupied_PMB_cells_index_offset = static_cast<int32_t>(framework::PersistentExtractionMode::MAX_OCCUPIED_PMB_CELLS_128);
				break;
			}
		}

		return num_occupied_PMB_cells_index_offset;
	}

	
	int32_t BaseRenderer::get_mode_dependent_meshlet_forward_rendering_shader_variant( bool is_meshopt_reference) const {
		bool uses_task_shaders = framework::MeshletForwardRenderPipelineMode::TASK_AND_MESH_SHADER_ONLINE_CULLING == renderer_conf.isosurface.meshlet_forward_rendering_pipeline_mode;

		return 
			  static_cast<int32_t>(framework::shader_variants_t::MAX_NUM_VERTICES_128_MAX_NUM_TRIANGLES_254) //use optimal meshlet size based on recommendations
			+ (is_meshopt_reference ? 0 : static_cast<int32_t>(framework::shader_variants_t::RENDER_ON_THE_FLY_EXTRACTED_MESHLETS) ) //use representation with one indirection less compared to offline created meshlets (see paper)
			+ ((uses_task_shaders) ? static_cast<int32_t>(framework::shader_variants_t::TASK_MESH_SHADER_COMBINATION) : 0)
			+ (use_NV_mesh_shader_extension_ ? 0 : MESH_SHADER_EXT_VARIANT_ADD_IN_INDEX)
			;
	}


	void BaseRenderer::render_frame() {
		//timing of the extraction parts is triggered inside the renderer, because of the different
		//stages which need to be timed
		extract_isosurfaces_();
		
		start_isosurface_draw_timer_();
		if (renderer_conf.general.render_isosurface) {
			render_single_opaque_isosurfaces_();
		}
		end_isosurface_draw_timer_();
	
		start_auxiliary_visualization_draw_timer_();
		if (renderer_conf.general.render_auxiliaries) {
			render_auxiliary_visualizations_();
		}
		end_auxiliary_visualization_draw_timer_();
	}

	void BaseRenderer::operate_post_frame() {
		last_frame_iso_value_ = cpu_extraction_parameters_buffer_->iso_value;
		last_frames_isosurface_indirect_compute_mode = renderer_conf.indirect_rendering_compute.compute_mode;
	}

	std::shared_ptr<BaseCamera> BaseRenderer::get_current_culling_reference_camera() const {
		assert(nullptr != culling_reference_camera_);
		return culling_reference_camera_;
	}

	std::shared_ptr<BaseCamera> BaseRenderer::get_current_point_of_view_camera() const {
		assert(nullptr != point_of_view_camera_);
		return point_of_view_camera_;
	}

	void BaseRenderer::set_clear_color(glm::vec4 clear_color) {
		clear_color_ = clear_color;
		needs_clear_color_update_ = true;
	}

	IndirectRenderingComputeMode BaseRenderer::get_active_indirect_rendering_compute_mode() const {
		return renderer_conf.indirect_rendering_compute.compute_mode;
	}

	void BaseRenderer::set_active_indirect_rendering_compute_mode(IndirectRenderingComputeMode const& indirect_rendering_compute_mode) {
		renderer_conf.indirect_rendering_compute.compute_mode = indirect_rendering_compute_mode;
	}

	uint32_t BaseRenderer::get_number_of_cameras() const {
		return cameras_.size();
	}

	int32_t BaseRenderer::get_active_culling_reference_camera_index() const {
		return active_culling_reference_camera_idx_;
	}

	void BaseRenderer::set_active_culling_reference_camera_by_index(uint32_t camera_to_become_main_index) {
		assert(camera_to_become_main_index >= 0 && camera_to_become_main_index < get_number_of_cameras() );
		culling_reference_camera_ = cameras_[camera_to_become_main_index];

		active_culling_reference_camera_idx_ = camera_to_become_main_index;
	}

	int32_t BaseRenderer::get_active_point_of_view_camera_index() const {
		return active_point_of_view_camera_idx_;
	}
	void BaseRenderer::set_active_point_of_view_camera_by_index(uint32_t camera_to_become_point_of_view_index) {
		assert(camera_to_become_point_of_view_index >= 0 && camera_to_become_point_of_view_index < get_number_of_cameras());

		point_of_view_camera_ = cameras_[camera_to_become_point_of_view_index];

		active_point_of_view_camera_idx_ = camera_to_become_point_of_view_index;
	}

	void BaseRenderer::set_active_model_yaw_angle_in_degree(float yaw_angle) {
		if (!yaw_pitch_roll_angles_in_degree_.empty()) {
			yaw_pitch_roll_angles_in_degree_.front()[YAW_ARRAY_IDX] = yaw_angle;
		}
		if (nullptr != cpu_model_buffer_) {
			cpu_model_buffer_->is_dirty = true;
		}
	}

	void BaseRenderer::set_active_model_pitch_angle_in_degree(float pitch_angle) {
		if (!yaw_pitch_roll_angles_in_degree_.empty()) {
			yaw_pitch_roll_angles_in_degree_.front()[PITCH_ARRAY_IDX] = pitch_angle;
		}
		if (nullptr != cpu_model_buffer_) {
			cpu_model_buffer_->is_dirty = true;
		}
	}

	void BaseRenderer::set_active_model_roll_angle_in_degree(float roll_angle) {
		if (!yaw_pitch_roll_angles_in_degree_.empty()) {
			yaw_pitch_roll_angles_in_degree_.front()[ROLL_ARRAY_IDX] = roll_angle;
		}
		if (nullptr != cpu_model_buffer_) {
			cpu_model_buffer_->is_dirty = true;
		}
	}

	void BaseRenderer::set_active_model_yaw_pitch_roll_angles_in_degree(float yaw_angle, float pitch_angle, float roll_angle) {
		set_active_model_yaw_angle_in_degree(yaw_angle);
		set_active_model_pitch_angle_in_degree(pitch_angle);
		set_active_model_roll_angle_in_degree(roll_angle);
	}

	float BaseRenderer::get_active_model_yaw_angle_in_degree() const {
		if(!yaw_pitch_roll_angles_in_degree_.empty()) {
			return yaw_pitch_roll_angles_in_degree_[0][YAW_ARRAY_IDX];
		}

		return 0.0f;
	}

	float BaseRenderer::get_active_model_pitch_angle_in_degree() const {
		if (!yaw_pitch_roll_angles_in_degree_.empty()) {
			return yaw_pitch_roll_angles_in_degree_[0][PITCH_ARRAY_IDX];
		}

		return 0.0f;
	}

	float BaseRenderer::get_active_model_roll_angle_in_degree() const {
		if (!yaw_pitch_roll_angles_in_degree_.empty()) {
			return yaw_pitch_roll_angles_in_degree_[0][ROLL_ARRAY_IDX];
		}

		return 0.0f;
	}

	glm::vec3 BaseRenderer::get_active_model_yaw_pitch_roll_angles_in_degree() const {
		return {
			get_active_model_yaw_angle_in_degree(),
			get_active_model_pitch_angle_in_degree(),
			get_active_model_roll_angle_in_degree()
		};
	}

	void BaseRenderer::create_shader_programs_() {
		create_task_mesh_shader_based_isosurface_extraction_shader_programs_();
		create_persistent_isosurface_extraction_shader_programs_();
	}

	void BaseRenderer::set_reextract_always(bool reextract_always_state) {
		reextract_always_ = reextract_always_state;
	}

	void BaseRenderer::force_reextration_for_next_frame() {
		needs_to_reextract_isosurface_next_frame_ = true;
	}

	void BaseRenderer::extract_isosurfaces_() {
		if ( 
			(!reextract_always_) &&
			(!needs_to_reextract_isosurface_next_frame_) &&
			(last_frame_iso_value_ == cpu_extraction_parameters_buffer_->iso_value) 
			) {
			return;
		}

		switch (renderer_conf.indirect_rendering_compute.compute_mode) {
			case IndirectRenderingComputeMode::COMPUTE_SHADER_BASED_EXTRACTION:
			{
				extraction_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_();
				break;
			}
			case IndirectRenderingComputeMode::TASK_MESH_SHADER_BASED_EXTRACTION: {
				extraction_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_();
				break;
			}
			case IndirectRenderingComputeMode::MESHOPT_OFFLINE_MESHLET_COMPUTATION: {

				auto const saved_compute_shader_path_extraction_mode = renderer_conf.indirect_rendering_compute.compute_shader_path_extraction_mode;

				renderer_conf.indirect_rendering_compute.compute_shader_path_extraction_mode = ComputeShaderExtractionMode::SINGLE_INDEX_ARRAY;

				extraction_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_(); //first extract a 32bit indexed mesh ...
					
				create_offline_optimized_meshlets_from_extracted_isosurface(); //... then download and consume the result with meshoptimizer

				renderer_conf.indirect_rendering_compute.compute_shader_path_extraction_mode = saved_compute_shader_path_extraction_mode;
				break;
			}
		}

		collect_extracted_geometry_statistics_();
		

		needs_to_reextract_isosurface_next_frame_ = false;
	}

	void BaseRenderer::render_single_opaque_isosurfaces_() {
		switch (renderer_conf.indirect_rendering_compute.compute_mode) {
			case IndirectRenderingComputeMode::COMPUTE_SHADER_BASED_EXTRACTION:
			{
				render_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_();
				break;
			}
			case IndirectRenderingComputeMode::TASK_MESH_SHADER_BASED_EXTRACTION: {
				render_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_();
				break;
			}
			case IndirectRenderingComputeMode::MESHOPT_OFFLINE_MESHLET_COMPUTATION: {
				render_pass_isosurface_indirectly_persistent_extraction_offline_created_meshlet_based_();
				break;
			}

			break;
																					
		}
	}

	void BaseRenderer::render_coordinate_axes_visualization_() {}

	void BaseRenderer::render_auxiliary_visualizations_() {
		if (renderer_conf.aux_vis.render_coordinate_axes) {
			render_coordinate_axes_visualization_();
		}

		if (renderer_conf.aux_vis.render_culling_reference_camera_frustum) {
			render_culling_reference_camera_frustum_();
		}
		if (renderer_conf.aux_vis.render_volume_boundaries) {
			render_volume_boundaries_();
		}
	}

	double BaseRenderer::get_total_elapsed_seconds() const {
		return total_elapsed_seconds_;
	}

	void BaseRenderer::set_isovalue(float iso_value) {
		assert(nullptr != cpu_extraction_parameters_buffer_);

		try_assign_new_isovalue_(iso_value);
	}

	float BaseRenderer::get_isovalue() const {
		assert(nullptr != cpu_extraction_parameters_buffer_);

		return cpu_extraction_parameters_buffer_->iso_value;
	}

	void BaseRenderer::set_shading_base_color(glm::vec3 const& shading_base_color) {
		assert(nullptr != cpu_draw_parameters_buffer_);

		try_assign_new_shading_base_color_(shading_base_color);
	}

	glm::vec3 BaseRenderer::get_shading_base_color() const {
		assert(nullptr != cpu_draw_parameters_buffer_);

		return cpu_draw_parameters_buffer_->shading_base_color;
	}

	void BaseRenderer::increase_isovalue(float increment) {
		assert(nullptr != cpu_extraction_parameters_buffer_);
		float const potential_new_isovalue = std::min(increment + cpu_extraction_parameters_buffer_->iso_value, 1.0f);
		try_assign_new_isovalue_(potential_new_isovalue);
	}
	void BaseRenderer::decrease_isovalue(float decrement) {
		assert(nullptr != cpu_extraction_parameters_buffer_);
		float const potential_new_isovalue = std::max(cpu_extraction_parameters_buffer_->iso_value - decrement, 0.0f);
		try_assign_new_isovalue_(potential_new_isovalue);
	}

	void BaseRenderer::try_assign_new_isovalue_(float new_iso_value) {
		if (0 != std::memcmp(&new_iso_value, &cpu_extraction_parameters_buffer_->iso_value, sizeof(float))) {
			cpu_extraction_parameters_buffer_->iso_value = new_iso_value;
			cpu_extraction_parameters_buffer_->is_dirty = true;

			force_reextration_for_next_frame();
		}
	}

	void BaseRenderer::try_assign_new_shading_base_color_(glm::vec3 const& new_shading_base_color) {
		if (0 != std::memcmp(&new_shading_base_color, &cpu_draw_parameters_buffer_->shading_base_color, sizeof(glm::vec3))) {
			std::memcpy(
				glm::value_ptr(cpu_draw_parameters_buffer_->shading_base_color),
				glm::value_ptr(new_shading_base_color), 
				sizeof(glm::vec3));
			cpu_draw_parameters_buffer_->is_dirty = true;
		}
	}

	void BaseRenderer::toggle_auto_rotation() {
		auto_rotate_ = !auto_rotate_;
	}

	void BaseRenderer::toggle_render_isosurface() {
		renderer_conf.general.render_isosurface = !renderer_conf.general.render_isosurface;
	}

	void BaseRenderer::toggle_render_auxiliaries() {
		renderer_conf.general.render_auxiliaries = !renderer_conf.general.render_auxiliaries;
	}


	per_app_start_performance_statistics BaseRenderer::get_per_app_start_performance_statistics() const {
		return per_app_start_performance_stats_;
	}

	per_frame_statistics BaseRenderer::get_per_frame_statistics() const {
		return per_frame_performance_stats_;
	}

	BlockOccupancyMode BaseRenderer::get_block_occupancy_mode() const {
		return block_occupancy_mode_;
	}

	void BaseRenderer::set_block_occupancy_mode(BlockOccupancyMode const& block_occupancy_mode_to_set) {
		block_occupancy_mode_ = block_occupancy_mode_to_set;
	}

	float BaseRenderer::get_last_frame_iso_value_() const {
		return last_frame_iso_value_;
	}

	void BaseRenderer::set_max_num_vertices_for_forward_meshlet_rendering(int32_t max_num_meshlet_vertices_to_set) {
		max_num_vertices_in_freely_defineable_mode_ = max_num_meshlet_vertices_to_set;
	}

	int32_t BaseRenderer::get_max_num_vertices_for_forward_meshlet_rendering() const {
		return max_num_vertices_in_freely_defineable_mode_;
	}

	void BaseRenderer::set_max_num_triangles_for_forward_meshlet_rendering(int32_t max_num_triangles_to_set) {
		max_num_triangles_in_freely_defineable_mode_ = max_num_triangles_to_set;
	}

	int32_t BaseRenderer::get_max_num_triangles_for_forward_meshlet_rendering() const {
		return max_num_triangles_in_freely_defineable_mode_;
	}

#undef ROLL_ARRAY_IDX
#undef PITCH_ARRAY_IDX
#undef YAW_ARRAY_IDX

}