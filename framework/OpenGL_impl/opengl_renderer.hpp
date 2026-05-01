#ifndef FRAMEWORK_OPENGL_RENDERER_HPP
#define FRAMEWORK_OPENGL_RENDERER_HPP


#include "opengl_UBOs.hpp"

#include "opengl_gpu_timer.hpp"

#include "../base_renderer.hpp"
#include "opengl_volume.hpp"
#include "../framework_platform.hpp"


namespace framework {
	class OpenGLGraphicsContext;
	
	
	class FRAMEWORK_DLL_EXPORT OpenGLRenderer : public BaseRenderer {
	public:
		OpenGLRenderer(int32_t preconfigured_index_buffer_size_in_MB = -1, int32_t preconfigured_position_buffer_size_in_MB = -1, int32_t preconfigured_normal_buffer_size_in_MB = -1 );
		~OpenGLRenderer() override = default;

		void register_volume_dataset(std::string const& resource_path, glm::ivec3 const& persistent_extraction_block_size) override;

	
		void operate_pre_frame() override;
		void operate_post_frame() override;

		void finalize_frame() override;
		void register_gui() override;
		void start_gui_frame() override;
		void end_gui_frame() override;
	private:
		virtual void map_cached_block_geometry_flag_buffer_() {};
		virtual void unmap_cached_block_geometry_flag_buffer_() {};

		virtual void map_buffers_pre_extraction_() {};
		virtual void unmap_buffers_post_extraction_() {};

	
		std::unique_ptr<BaseGPUTimer> create_new_gpu_timer_instance_(std::string const& timer_name) const override;

	
		camera_descriptor_t create_default_camera_descriptor_(glm::ivec2 const& window_resolution, glm::vec3 const& culling_camera_reference_position) const override;

		void initialize_context_(glm::ivec2 const& requested_window_resolution) override;
		void initialize_API_specific_graphics_objects_() override;

		void initialize_final_frame_buffers_() override;

		void initialize_UBOs_() override;
		void initialize_culling_reference_camera_buffer_();
		void initialize_point_of_view_camera_buffer_();
		void initialize_model_buffer_();
		void initialize_combined_model_culling_reference_camera_buffer_();
		void initialize_combined_model_point_of_view_camera_buffer_();
		void initialize_draw_parameters_buffer_();
		void initialize_extraction_parameters_buffer_();
		void initialize_render_frame_descriptor_buffer_();

		void initialize_atomic_counters_() override;

		void update_UBOs_() override;

		void update_culling_reference_camera_buffer_();
		void update_point_of_view_camera_buffer_();
		void update_model_buffer_();
		void update_combined_model_culling_reference_camera_buffer_();
		void update_combined_model_point_of_view_camera_buffer_();
		void update_draw_parameters_buffer_();
		void update_extraction_parameters_buffer_();
		void update_render_frame_descriptor_buffer_();


		std::shared_ptr<OpenGLGraphicsContext> opengl_context_ = nullptr;

		uint32_t draw_FBO_ = 0;
		uint32_t draw_buffer_color_attachment_ = 0;
		uint32_t draw_buffer_depth_attachment_ = 0;



	//compute passes for indirect isosurface extraction
	
		void extraction_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_() override;
		void extraction_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_() override;
		void create_offline_optimized_meshlets_from_extracted_isosurface() override;

		void collect_extracted_geometry_statistics_() override;

	
		void internal_render_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_();
		void internal_render_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_();

		void render_pass_isosurface_indirectly_persistent_extraction_compute_shader_based_() override;
		void render_pass_isosurface_indirectly_persistent_extraction_mesh_shader_based_() override;
		void render_pass_isosurface_indirectly_persistent_extraction_offline_created_meshlet_based_() override;

		void render_coordinate_axes_visualization_() override;
		void render_culling_reference_camera_frustum_() override;
		void render_volume_boundaries_() override;

	
		void start_isosurface_draw_timer_() override;
		void end_isosurface_draw_timer_() override;
		void start_auxiliary_visualization_draw_timer_() override;
		void end_auxiliary_visualization_draw_timer_() override;

		// buffer for UBO binding point 4
		std::unique_ptr<gpu_opengl_culling_reference_camera_buffer>  gpu_opengl_culling_reference_camera_buffer_ = nullptr;
		// buffer for UBO binding point 5
		std::unique_ptr<gpu_opengl_point_of_view_camera_buffer>  gpu_opengl_point_of_view_camera_buffer_ = nullptr;
		// buffer for UBO binding point 6
		std::unique_ptr<gpu_opengl_model_buffer> gpu_opengl_model_buffer_ = nullptr;
		// buffer for UBO binding point 7
		std::unique_ptr<gpu_opengl_combined_model_culling_reference_camera_buffer> gpu_opengl_combined_model_culling_reference_camera_buffer_ = nullptr;
		// buffer for UBO binding point 8
		std::unique_ptr<gpu_opengl_combined_model_point_of_view_camera_buffer> gpu_opengl_combined_model_point_of_view_camera_buffer_ = nullptr;


		// buffer for UBO binding point 11
		std::unique_ptr<gpu_opengl_draw_parameters_buffer> gpu_opengl_draw_parameters_buffer_ = nullptr;
		// buffer for UBO binding point 12
		std::unique_ptr<gpu_opengl_extraction_parameters_buffer> gpu_opengl_extraction_parameters_buffer_ = nullptr;
		// buffer for UBO binding point 13
		std::unique_ptr<gpu_opengl_fbo_bindless_texture_adresses_buffer> gpu_opengl_fbo_bindless_texture_addresses_buffer_ = nullptr;
		// buffer for UBO binding point 14
		std::unique_ptr<gpu_opengl_render_frame_descriptor_buffer> gpu_opengl_render_frame_descriptor_buffer_ = nullptr;

	protected:
		//for meshlet marching cubes on the fly
		uint32_t gl_edgeTable_buffer_tbo_ = 0;
		uint32_t gl_unique_vertices_triTable_buffer_tbo_ = 0;
		uint32_t gl_total_numTris_table_buffer_tbo_ = 0;
		uint32_t gl_unique_numVerts_table_buffer_tbo_ = 0;

		uint32_t gl_16_bit_packed_tri_table_buffer_tbo_ = 0;


		uint32_t gl_edgeTable_buffer_ref_ = 0;
		uint32_t gl_unique_vertices_triTable_buffer_ref_ = 0;
		uint32_t gl_total_numTris_table_buffer_ref_ = 0;
		uint32_t gl_unique_numVerts_table_buffer_ref_ = 0;

		uint32_t gl_16_bit_packed_tri_table_buffer_ref_ = 0;


		mutable uint32_t currently_bound_indirect_draw_buffer = 0;

		typedef  struct {
			uint32_t  count;
			uint32_t  instanceCount;
			uint32_t  firstIndex;
			int32_t  baseVertex;
			uint32_t  baseInstance;
		} DrawElementsIndirectCommand;

		uint64_t num_byte_per_frame_block_based_indirect_draw_parameter_ssbo_ = 0;
		uint32_t per_frame_block_based_indirect_draw_parameter_ssbo_ = 0;




		uint32_t indirect_compute_buffer = 0;


	private:
		void reset_extraction_related_atomic_counter_values_() override {};
		void compute_min_max_block_texture_() override;

		virtual void clear_atomic_counter_buffer_(bool is_in_compute_shader_extraction_mode = true) const;
		virtual void compute_dense_occupancy_brick_indices_() const;
		std::pair<uint32_t, uint32_t> extract_trimesh_parallel_marching_blocks_(std::vector<uint32_t> const& num_occupied_blocks_host) override;
		// steps for triangle extraction


		void hotplug_new_vertex_and_triangle_counts_for_indirect_meshlet_rendering_mode();

	

		void execute_transient_isosurface_extraction_pipeline_(std::shared_ptr<shader_program> const& task_mesh_pipeline,
			std::shared_ptr<shader_program> const& task_mesh_pipeline_current_but_not_last,
			bool is_in_evaluation_mode);


	protected:
		virtual float retrieve_min_max_grid_computation_time_ms_();
		virtual float retrieve_dense_occupancy_brick_indices_computation_time_ms_();
		virtual float retrieve_isosurface_extraction_computation_time_ms_();
		virtual float retrieve_meshlet_descriptor_sorting_time_ms_();

	
		virtual void create_timer_query_objects_();
		virtual void destroy_timer_query_objects_();
		void create_UBOs_();

		void update_frame_counter_dependent_variable_UBO();
		void update_model_UBO_(glm::mat4 const& model_mat, std::shared_ptr<BaseCamera> const& reference_camera, std::shared_ptr<BaseCamera> const& active_camera);

		void allocate_texture_buffer_objects_();
		void allocate_shader_storage_buffer_objects_();
		virtual void allocate_storage_buffers_();

	
		void release_gpu_resources_() override;


		//gl_atomic_related
	private:
		uint32_t collapsed_atomic_counter_buffers = 0;

		
		uint32_t const NUM_REGION_SIZES_TO_COUNT = 3;
		uint32_t const NUM_VERTEX_COUNT_BINS_TO_COUNT = 128;
		uint32_t const NUM_TRIANGLE_COUNT_BINS_TO_COUNT = 248;
		uint32_t const NUM_JOINT_ATOMIC_COUNTERS = 1024 + NUM_REGION_SIZES_TO_COUNT + NUM_VERTEX_COUNT_BINS_TO_COUNT + NUM_TRIANGLE_COUNT_BINS_TO_COUNT;
		
		
		uint32_t const& parallel_marching_blocks_extracted_vertices_atomic_counter = collapsed_atomic_counter_buffers;
		uint32_t const& parallel_marching_blocks_extracted_indices_atomic_counter = collapsed_atomic_counter_buffers;
	};
}

#endif //FRAMEWORK_OPENGL_RENDERER_HPP