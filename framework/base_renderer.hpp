#ifndef FRAMEWORK_BASE_RENDERER_HPP
#define FRAMEWORK_BASE_RENDERER_HPP


#include "base_volume.hpp"
#include "base_camera.hpp"

#include "base_gpu_timer.hpp"

#include "base_renderer_types.hpp"

#include "framework_platform.hpp"
#include "base_graphics_context.hpp"

#include "available_shader_programs.hpp"
#include "CPU_UBO_types.hpp"

#include <meshoptimizer.h>

#include <GLFW/glfw3.h>

#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace framework {
	//recommendations for max. meshlet sizes by Arseny Kapoulkine: (see Readme.md of https://github.com/zeux/meshoptimizer)
	static size_t constexpr MAX_OFFLINE_CREATED_VERTICES_PER_MESHLET = 128;
	static size_t constexpr MAX_OFFLINE_CREATED_TRIANGLES_PER_MESHLET = 254;

	// timed once per app start, not per frame
	struct per_app_start_performance_statistics {
		double min_max_computation_time_ms = -1.0;

		void clear_all() {
			min_max_computation_time_ms = -1.0;
		}
	};

	struct per_pipeline_statistics_components {
		double min_max_computation_time_ms = -1.0;
		double dense_occupancy_computation_time_ms = 0.0;
		double occupancy_filter_time_ms = 0.0;
		double pure_extraction_time_ms = 0.0;
		double pure_isosurface_rendering_time_ms = 0.0;
		double pure_auxiliary_vis_rendering_time_ms = 0.0;

		double render_pass_one_time_ms = 0.0;
		double accumulated_frame_time_ms = 0.0;
	};

	struct separated_memory_footprint_t {
		int64_t indices = 0;
		int64_t vertex_positions = 0;
		int64_t vertex_normals = 0;
		int64_t block_descriptors_persistent_part = 0;
		int64_t block_descriptors_live_part = 0;
		int64_t meshlet_descriptors = 0;
		int64_t culling_information = 0;

		int64_t total = 0;
	};

	// the active members of the two unions below are selected by `renderer_timer_mode`
	// and `renderer_memory_footprint_mode` respectively
	struct per_frame_statistics {
		void clear_all() {
			std::memset(this, 0x0, sizeof(per_frame_statistics));
			renderer_timer_mode = timer_mode::GLOBAL_FRAME;
			renderer_memory_footprint_mode = memory_footprint_mode::NONE;
		}

		union time_measurement_mode {
			time_measurement_mode() : global_time_ms(0.0) {}
			per_pipeline_statistics_components per_component;
			double global_time_ms;
		} measured_time;

		union memory_footprint_measurement_mode {
			memory_footprint_measurement_mode() : total_amount_byte(0) {}
			separated_memory_footprint_t separated_memory_amounts_byte;
			int64_t total_amount_byte;
		} measured_memory_footprints;

		timer_mode renderer_timer_mode = timer_mode::GLOBAL_FRAME;
		memory_footprint_mode renderer_memory_footprint_mode = memory_footprint_mode::NONE;
	};

	class FRAMEWORK_DLL_EXPORT BaseRenderer {
	public:
		static bool NO_GUI;
		static void set_NO_GUI(bool value);

		struct camera_config {
			bool show_window = false;
		} camera_conf;

		struct model_config {
			bool show_window = false;
		} model_conf;

		struct renderer_config {
			struct aux_vis_states {
				bool render_coordinate_axes = true;
				bool render_culling_reference_camera_frustum = true;
				bool render_volume_boundaries = true;

			} aux_vis;

			struct general_states {
				bool render_isosurface = true;
				bool render_auxiliaries = true;
			} general;

			struct indirect_rendering_compute_settings {
				IndirectRenderingComputeMode compute_mode = IndirectRenderingComputeMode::TASK_MESH_SHADER_BASED_EXTRACTION;
				ComputeShaderExtractionMode compute_shader_path_extraction_mode = ComputeShaderExtractionMode::BLOCKWISE_16BIT_INDICES;
			} indirect_rendering_compute;

			struct isosurface_settings {
				MeshletForwardRenderPipelineMode meshlet_forward_rendering_pipeline_mode = MeshletForwardRenderPipelineMode::MESH_SHADER_ONLY_NO_CULLING;

				MeshletVisualizationMode persistent_meshlet_visualization_mode = MeshletVisualizationMode::PHONG_SHADED;
				IndexedMeshVisualizationMode persistent_indexed_mesh_visualization_mode = IndexedMeshVisualizationMode::PHONG_SHADED;
			} isosurface;
		};
		
		mutable renderer_config renderer_conf;

		BaseRenderer(int32_t preconfigured_index_buffer_size_in_MB = -1, int32_t preconfigured_position_buffer_size_in_MB = -1, int32_t preconfigured_normal_buffer_size_in_MB = -1);
		virtual ~BaseRenderer() = default;

		virtual void register_volume_dataset(std::string const& resource_path, glm::ivec3 const& persistent_extraction_block_size) = 0;

		std::shared_ptr<BaseCamera> get_current_culling_reference_camera() const;
		std::shared_ptr<BaseCamera> get_current_point_of_view_camera() const;

		void initialize(
						glm::ivec2 const& window_resolution, 
						glm::vec3 culling_camera_reference_position = { 0.0f, 0.0f, 4.6f }, 
						glm::vec3 pov_camera_reference_position = { -10.0f, 4.6f, -4.6f });
		void tear_down();

		void create_shader_programs();

		// once per frame methods
		virtual void operate_pre_frame();
		void render_frame();
		virtual void operate_post_frame();

		virtual void finalize_frame() = 0;
		virtual void register_gui() = 0;
		virtual void start_gui_frame();
		virtual void end_gui_frame();

		void build_meshlets_offline(uint32_t& in_out_vertex_count, uint32_t& in_out_index_count);

	
		void register_window_callbacks();

		// methods used by key callback
		double get_total_elapsed_seconds() const;

		per_app_start_performance_statistics get_per_app_start_performance_statistics() const;
		per_frame_statistics get_per_frame_statistics() const;

		void set_active_model_yaw_angle_in_degree(float yaw_angle);
		void set_active_model_pitch_angle_in_degree(float pitch_angle);
		void set_active_model_roll_angle_in_degree(float roll_angle);
		void set_active_model_yaw_pitch_roll_angles_in_degree(float yaw_angle, float pitch_angle, float roll_angle);

		float get_active_model_yaw_angle_in_degree() const;
		float get_active_model_pitch_angle_in_degree() const;
		float get_active_model_roll_angle_in_degree() const;
		glm::vec3 get_active_model_yaw_pitch_roll_angles_in_degree() const;

		void set_isovalue(float iso_value);
		float get_isovalue() const;
		void increase_isovalue(float increment);
		void decrease_isovalue(float decrement);

		void set_clear_color(glm::vec4 clear_color);

		void set_shading_base_color(glm::vec3 const& shading_base_color);
		glm::vec3 get_shading_base_color() const;

		void toggle_auto_rotation();

		void toggle_render_isosurface();
		void toggle_render_auxiliaries();

		void set_gpu_timer_mode(timer_mode gpu_timer_mode) {
			per_frame_performance_stats_.renderer_timer_mode = gpu_timer_mode;
		}

		timer_mode get_gpu_timer_mode() const {
			return per_frame_performance_stats_.renderer_timer_mode;
		}

		void set_gpu_memory_footprint_mode(memory_footprint_mode gpu_memory_footprint_mode) {
			per_frame_performance_stats_.renderer_memory_footprint_mode = gpu_memory_footprint_mode;
		}

		memory_footprint_mode get_gpu_memory_footprint_mode() const {
			return per_frame_performance_stats_.renderer_memory_footprint_mode;
		}


		cpu_culling_reference_camera_buffer get_culling_reference_camera_cpu_buffer() const;
		cpu_combined_model_culling_reference_camera_buffer get_combined_model_culling_reference_camera_cpu_buffer() const;

		cpu_point_of_view_camera_buffer get_point_of_view_camera_cpu_buffer() const;
		cpu_combined_model_point_of_view_camera_buffer get_combined_model_point_of_view_camera_cpu_buffer() const;

	private:
		void try_assign_new_isovalue_(float new_iso_value);
		void try_assign_new_shading_base_color_(glm::vec3 const& new_shading_base_color);

		virtual std::unique_ptr<BaseGPUTimer> create_new_gpu_timer_instance_(std::string const& timer_name) const = 0;
		
	//mode getter and setter
	public:
		IndirectRenderingComputeMode get_active_indirect_rendering_compute_mode() const;
		void set_active_indirect_rendering_compute_mode(IndirectRenderingComputeMode const& indirect_rendering_compute_mode);

		uint32_t get_number_of_cameras() const;

		int32_t get_active_culling_reference_camera_index() const;
		void set_active_culling_reference_camera_by_index(uint32_t camera_to_become_main_index);

		int32_t get_active_point_of_view_camera_index() const;
		void set_active_point_of_view_camera_by_index(uint32_t camera_to_become_point_of_view_index);

	protected:
		void add_volume_dataset_(std::shared_ptr<BaseVolume> const& volume_dataset);

		void initialize_default_culling_reference_camera_(glm::vec3 culling_camera_reference_position);
		void initialize_additional_cameras_(glm::vec3 additional_camera_reference_position);

		void add_existing_camera_to_database_(std::shared_ptr<BaseCamera> const& camera_to_become_point_of_view_cam);
	
		// we keep a reference to the "main camera", which is the default we render through ...
		std::shared_ptr<BaseCamera> culling_reference_camera_;
		int32_t active_culling_reference_camera_idx_ = 0;

		camera_descriptor_t default_culling_reference_camera_parameters_;

		glm::ivec2 window_resolution_ = { -1, -1 };

		available_shader_programs_t framework_shader_programs_;

		bool needs_clear_color_update_ = true;
		glm::vec4 clear_color_ = { 1.0f, 0.0f, 0.0f, 1.0f };

		int32_t total_num_extracted_meshlets_ = 0;

	// once per app methods
	
		virtual void initialize_API_specific_graphics_objects_() = 0;

		//void apply_command_line_parameter_dependent_start_UBO_values_();
	private:

		virtual camera_descriptor_t create_default_camera_descriptor_(glm::ivec2 const& window_resolution, glm::vec3 const& culling_camera_reference_position) const = 0;

		virtual void initialize_context_(glm::ivec2 const& requested_window_resolution) = 0;

		virtual void initialize_final_frame_buffers_() = 0;
		virtual void initialize_UBOs_() = 0;

		virtual void initialize_atomic_counters_() = 0;

		void precompute_auxiliary_structures_();

		virtual void update_UBOs_() = 0;

		void assign_culling_reference_camera_(std::shared_ptr<BaseCamera> const& camera_to_become_main_cam);
		void assign_point_of_view_camera_(std::shared_ptr<BaseCamera> const& camera_to_become_point_of_view_cam);
		virtual void reserve_space_for_number_of_volume_datasets_(uint32_t number_of_different_volume_datasets) final;

	protected: //to be called by the specific API implementations
		void initialize_render_frame_descriptor_buffer_(cpu_render_frame_descriptor_buffer& cpu_memory_render_frame_descriptor_buffer_to_initialize);
		void update_render_frame_descriptor_buffer_cpu_memory_();
		void initialize_model_buffer_cpu_memory_(cpu_model_buffer& cpu_memory_model_buffer_to_initialize);
		void update_model_buffer_cpu_memory_();
		void initialize_culling_reference_camera_buffer_cpu_memory_(cpu_culling_reference_camera_buffer& cpu_memory_culling_reference_camera_buffer_to_initialize);
		void update_culling_reference_camera_buffer_cpu_memory_();
		void initialize_point_of_view_camera_buffer_cpu_memory_(cpu_point_of_view_camera_buffer& cpu_memory_point_of_view_buffer_to_initialize);
		void update_point_of_view_camera_buffer_cpu_memory_();
		void initialize_combined_model_culling_reference_camera_buffer_cpu_memory_(cpu_combined_model_culling_reference_camera_buffer& cpu_memory_combined_culling_reference_camera_buffer_to_initialize);
		void update_combined_model_culling_reference_camera_buffer_cpu_memory_();
		void initialize_combined_model_point_of_view_camera_buffer_cpu_memory_(cpu_combined_model_point_of_view_camera_buffer& cpu_memory_combined_point_of_view_buffer_to_initialize);
		void update_combined_model_point_of_view_camera_buffer_cpu_memory_();

		void initialize_draw_parameters_buffer_(cpu_draw_parameters_buffer& cpu_memory_draw_parameters_buffer_to_initialize);
		void update_draw_parameters_buffer_cpu_memory_();
		void initialize_extraction_parameters_buffer_(cpu_extraction_parameters_buffer& cpu_memory_extraction_parameters_buffer_to_initialize);
		void update_extraction_parameters_buffer_cpu_memory_();
	public:
		
	// member variables
	protected:
		std::vector<std::shared_ptr<BaseVolume>> volumes_;

		std::vector<glm::vec3> yaw_pitch_roll_angles_in_degree_ = { { 0.0f, 0.0f, 0.0f } };

		uint32_t overall_num_blocks_in_volume_ = 0;
		uint32_t num_compactification_workgroups_to_launch_for_all_blocks_ = 0;
		uint32_t num_output_building_workgroups_to_launch_for_all_blocks_ = 0;

		uint32_t num_byte_per_extracted_triangle_index_ = 0;
		uint32_t num_byte_offline_built_meshlets_ = 0;

	private:

		std::vector<std::shared_ptr<BaseCamera>> cameras_;



		// ... and a POV_camera. The "point of view"-camera can be the same as the culling_reference_camera itself, or any other external camera we use
		// to visualize culling-based rendering effects
		std::shared_ptr<BaseCamera> point_of_view_camera_ = nullptr;
		int32_t active_point_of_view_camera_idx_ = 0;

	
		IndirectRenderingComputeMode last_frames_isosurface_indirect_compute_mode = IndirectRenderingComputeMode::INVALID_COMPUTE_MODE;

	
		void update_total_elapsed_renderer_time_();

	
		virtual void create_shader_programs_();

	
	   void create_task_mesh_shader_based_isosurface_extraction_shader_programs_();
	   void create_persistent_isosurface_extraction_shader_programs_();

	
		//only actually used for indirect computation modes
		void extract_isosurfaces_();
		void render_single_opaque_isosurfaces_();
		void render_auxiliary_visualizations_();

		virtual void reset_extraction_related_atomic_counter_values_() = 0;
		virtual void compute_min_max_block_texture_() = 0;
		virtual std::pair<uint32_t, uint32_t> extract_trimesh_parallel_marching_blocks_(std::vector<uint32_t> const& num_occupied_blocks_host) = 0;

	public:
		/* although except for measurement scenarios we do not want to re - extract isosurfaces when we have them extracted already, online mode - switches
		   make it required to force the re-extraction, because the internal representation differs.
		*/
		void set_reextract_always(bool reextract_always_state);
		void force_reextration_for_next_frame();
	private:
		virtual void extraction_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_() = 0;
		virtual void extraction_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_() = 0;
		virtual void create_offline_optimized_meshlets_from_extracted_isosurface() = 0;

		virtual void collect_extracted_geometry_statistics_() = 0;

	// isosurface rendering modes
	
		virtual void render_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_() = 0;
		virtual void render_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_() = 0;
		virtual void render_pass_isosurface_indirectly_persistent_extraction_offline_created_meshlet_based_() = 0;

	// auxiliary visualization routines
	
		virtual void render_coordinate_axes_visualization_() = 0;
		virtual void render_culling_reference_camera_frustum_() = 0;
		virtual void render_volume_boundaries_() = 0;

	
		virtual void start_isosurface_draw_timer_() {};
		virtual void end_isosurface_draw_timer_() {}
		virtual void start_auxiliary_visualization_draw_timer_() {};
		virtual void end_auxiliary_visualization_draw_timer_() {}

	
		std::chrono::time_point<std::chrono::system_clock> latest_timestamp_ = std::chrono::system_clock::now();

	protected:
		double total_elapsed_seconds_ = 0.0;
		double total_elapsed_seconds_for_auto_rotation_ = 0.0;


		mutable bool needs_to_reextract_isosurface_next_frame_ = true;
		bool reextract_always_ = false;
		mutable int32_t num_offline_created_meshlets_ = -1;  // actual_meshlet_count


		mutable std::vector<uint32_t> remapped_indices_;
		mutable std::vector<glm::vec3> remapped_positions_;
		mutable std::vector<glm::vec3> remapped_normals_;
		mutable std::vector<uint32_t> index_remap_table_; // allocate temporary memory for the remap table
		mutable std::vector<::meshopt_Meshlet> built_meshlets_cpu_;
		mutable std::vector<glm::vec4> meshlet_bounding_spheres_;

		mutable std::vector<unsigned int> meshlet_vertex_indices_;
		mutable std::vector<unsigned char> meshlet_triangle_indices_;

		mutable std::vector<glm::vec3> positions_cpu_;
		mutable std::vector<glm::vec3> normals_cpu_;

		mutable std::vector<uint32_t> indices_cpu_;

	private: 		
		// CPU buffer associated with GPU buffer for UBO binding point 4
		cpu_culling_reference_camera_buffer*  cpu_culling_reference_camera_buffer_ = nullptr;
		// CPU buffer associated with GPU buffer for UBO binding point 5
		cpu_point_of_view_camera_buffer* cpu_point_of_view_camera_buffer_ = nullptr;
		// CPU buffer associated with GPU buffer for UBO binding point 6
		cpu_model_buffer* cpu_model_buffer_ = nullptr;
		// CPU buffer associated with GPU buffer for UBO binding point 7
		cpu_combined_model_culling_reference_camera_buffer* cpu_combined_model_culling_reference_camera_buffer_ = nullptr;
		// CPU buffer associated with GPU buffer for UBO binding point 8
		cpu_combined_model_point_of_view_camera_buffer* cpu_combined_model_point_of_view_camera_buffer_ = nullptr;

		// CPU buffer associated with GPU buffer for UBO binding point 12
		cpu_draw_parameters_buffer* cpu_draw_parameters_buffer_ = new cpu_draw_parameters_buffer();
		// CPU buffer associated with GPU buffer for UBO binding point 13
		cpu_extraction_parameters_buffer* cpu_extraction_parameters_buffer_ = new cpu_extraction_parameters_buffer();;
		// CPU buffer associated with GPU buffer for UBO binding point 14
		cpu_fbo_bindless_texture_adresses_buffer* cpu_fbo_bindless_texture_addresses_buffer_ = nullptr;
		// CPU buffer associated with GPU buffer for UBO binding point 15
		cpu_render_frame_descriptor_buffer* cpu_render_frame_descriptor_buffer_ = nullptr;

	//GUI INTERACTABLES
	protected:
		bool are_UBOs_allocated_ = false;
		bool are_auxiliary_structures_allocated_ = false;
		bool are_API_specific_graphics_objects_initialized = false;

	private:
		bool auto_rotate_ = false;


	protected:
		std::shared_ptr<BaseGraphicsContext> base_context_ = nullptr;

	// GPU timers
	private:		
		protected:
		std::unique_ptr<BaseGPUTimer> complete_frame_without_gui_rendering_timer_ = nullptr;

		std::unique_ptr<BaseGPUTimer> per_component_timers__dense_occupancy_computation_timer_ = nullptr;
		std::unique_ptr<BaseGPUTimer> per_component_timers__extraction_times_isosurface_extraction_timer_ = nullptr;
		std::unique_ptr<BaseGPUTimer> per_component_timers__draw_times_isosurface_draw_timer_ = nullptr;
		std::unique_ptr<BaseGPUTimer> per_component_timers__draw_times_aux_vis_draw_timer_ = nullptr;

		std::unique_ptr<BaseGPUTimer> per_application_timers__min_max_volume_computation_timer_ = nullptr;

	
		per_app_start_performance_statistics per_app_start_performance_stats_;
		per_frame_statistics per_frame_performance_stats_;

	
		uint64_t num_bytes_to_conservatively_allocate_pos_VBO_ = 0;
		uint32_t persistent_extraction_pos_VBO_ = 0;

		uint64_t num_bytes_to_conservatively_allocate_nor_VBO_ = 0;
		uint32_t persistent_extraction_nor_VBO_ = 0;

		uint64_t num_bytes_to_conservatively_allocate_EBO__32bit_indices = 0;
		uint32_t persistent_extraction_EBO__indices = 0;

		uint64_t num_bytes_to_conservatively_allocate_EBO__16bit_indices = 0;
		uint32_t persistent_extraction_EBO__16bit_indices = 0;


		//meshlet extraction related
		uint64_t num_bytes_to_conservatively_allocate_meshlet_vertex_offset_and_count__index_offset_and_count__buffer_ = 0;
		uint32_t persistent_extraction_meshlet_vertex_offset_and_count__index_offset_and_count__buffer_ = 0;

		//block extraction related
		uint64_t num_bytes_meshlet_vertex_indices_buffer_32_bit_ = 0;
		uint32_t meshoptimizer_created_meshlet_vertex_indices_buffer_32bit_ = 0;

		uint64_t num_bytes_meshlet_culling_info_ = 0;
		uint32_t meshlet_culling_info_buffer_ = 0;


		uint64_t num_bytes_occupied_block_indices_ssbo_ = 0;
		uint32_t occupied_block_indices_ssbo_ = 0;

		uint64_t num_bytes_dense_meshlet_group_offset_and_length_ssbo_ = 0;
		uint32_t dense_meshlet_group_offset_and_length_ssbo_ = 0;


		uint32_t VAO_solid_SOA = 0;

		uint32_t VAO_solid_SOA_with_index_buffer__position_only = 0;
		uint32_t VAO_solid_SOA_with_index_buffer__position_and_normal = 0;

		int32_t preconfigured_index_buffer_size_in_MB_ = -1;
		int32_t preconfigured_position_buffer_size_in_MB_ = -1;
		int32_t preconfigured_normal_buffer_size_in_MB_ = -1;

	private:
		uint64_t num_extracted_vertices_ = 0;
		uint64_t num_extracted_primitives_ = 0;

	
		int32_t max_num_vertices_in_freely_defineable_mode_ = 256;
		int32_t max_num_triangles_in_freely_defineable_mode_ = 512;

	public:
		void set_max_num_vertices_for_forward_meshlet_rendering(int32_t max_num_vertices_to_set);
		int32_t get_max_num_vertices_for_forward_meshlet_rendering() const;

		void set_max_num_triangles_for_forward_meshlet_rendering(int32_t max_num_triangles_to_set);
		int32_t get_max_num_triangles_for_forward_meshlet_rendering() const;

		bool is_running() const;

		void set_use_NV_mesh_shader_extension(bool use_NV_mesh_shader_extension);
		void set_max_num_occupied_blocks_for_region_analysis_kernels(int32_t max_num_occupied_blocks_for_region_analysis_kernels_to_set);

		void precompile_shaders();

	protected:
		std::vector<std::string> latest_hotplug_request_;

	private:
		float last_frame_iso_value_ = std::numeric_limits<float>::max();

		BlockOccupancyMode block_occupancy_mode_ = BlockOccupancyMode::MIN_MAX_BINARY_OCCUPANCY;
	protected:
		BlockOccupancyMode get_block_occupancy_mode() const;
		void set_block_occupancy_mode(BlockOccupancyMode const& block_occupancy_mode_to_set);

		float get_last_frame_iso_value_() const;

	
		std::unique_ptr<VolumeDescriptor> main_volume_descriptor_ = nullptr;

		bool draw_gui_ = true;
		bool use_NV_mesh_shader_extension_ = true;
		int32_t max_num_occupied_blocks_for_region_analysis_kernels_ = 96;
		int32_t max_num_task_shader_workgroup_threads_extraction_kernel = 32;
		int32_t max_num_mesh_shader_workgroup_threads_extraction_kernel = 32;

		int32_t get_mode_dependent_mesh_shader_extraction_shader_variant() const;
		int32_t get_mode_dependent_meshlet_forward_rendering_shader_variant(bool x = false) const;

	private:
		virtual void release_gpu_resources_() = 0;

		bool is_running_ = true;
		bool precompile_shaders_ = false;
	
		void key_callback_(GLFWwindow* window, int key, int scancode, int action, int mod);
		void scroll_callback_(GLFWwindow* window, double x_offset, double y_offset);
		void mouse_moved_callback_(GLFWwindow* window, double mouse_pos_x, double mouse_pos_y);
		void mouse_click_callback_(GLFWwindow* window, int button, int action, int mods);
	};
}

#endif //FRAMEWORK_BASE_RENDERER_HPP