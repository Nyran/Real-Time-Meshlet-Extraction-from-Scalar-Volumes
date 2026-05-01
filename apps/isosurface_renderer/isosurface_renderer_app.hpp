#ifndef ISOSURFACE_RENDERER_APP
#define ISOSURFACE_RENDERER_APP


#include "framework_cmake_config.hpp"
#include "framework/framework_platform.hpp"
#include "framework/base_renderer.hpp"


//external dependencies
#include <GLFW/glfw3.h>
#include <glm/matrix.hpp>
#include <glm/gtx/string_cast.hpp>

#include <memory>
#include <numeric>
#include <thread>


struct initial_rendering_mode_settings_t {
	bool use_task_mesh_shader_pipeline_for_persistent_extraction = false;
	bool perform_reference_meshlet_construction = false;
};


	class IsosurfaceRendererApp {
	public:
		struct AppRunDescriptor {
			bool precompile_shaders = false;
			bool disable_shader_cache = false;

			glm::ivec2 window_resolution = { 3840, 2160 };
			float start_iso_value = 0.2f;

			int32_t initial_hotkey_rotation = 0;
			std::array<float,3> shading_base_color = { 108.0f / 255.0f, 86.0f / 255.0f, 180.0f / 255.0f };

			std::array<float, 3> wireframe_base_color = { 0.2f, 0.2f, 0.2f };

			glm::vec3 background_color = { 1.0f, 1.0f, 1.0f};
			glm::vec3 last_frames_background_color = { 1.0f, 1.0f, 1.0f };

			int32_t	swap_interval = 0;

			bool is_in_evaluation_mode = false;

			framework::initial_model_transformation_descriptor_t initial_model_transform_settings;

			struct PersistentGeometryBufferSettings {
				int32_t position_buffer_size_in_MB = -1;
				int32_t normal_buffer_size_in_MB = -1;
				int32_t index_buffer_size_in_MB = -1;
			} geom_buffer_settings;

			struct EvaluationSettings {
				float end_iso_value = std::numeric_limits<float>::max();
				bool evaluate_individual_pipeline_timing_components = false;
				bool evaluate_individual_pipeline_memory_components = false;
				float iso_range_evalation_step_size = 0.05f;
				uint32_t evaluation_samples_per_measurement = 10u;

				uint32_t num_PMB_cells_allowed_per_mesh_shading_workgroup = 64;

				bool perform_iso_range_evaluation = false;

				bool always_reextract = false;
				bool log_memory_footprint_after_extraction = false;

				std::string evaluation_base_name = "undefined";

				std::string evaluation_base_path = "./";


				bool extract_bounding_volumes = false;
				bool pass_extraction_block_ids_as_implicit_AABBs = false;

				glm::ivec3 extraction_block_sizes_for_labeling = {0, 0, 0};


			} eval_settings;

		};

	
		IsosurfaceRendererApp(AppRunDescriptor const& run_descriptor);
		~IsosurfaceRendererApp();

		void initialize_framework(AppRunDescriptor const& run_descriptor);
		void tear_down_framework();

		void write_global_frame_time_log_file_();
		void write_per_component_frame_time_log_file_();
		void write_global_memory_footprint_stats_log_file_();
		void write_per_component_memory_footprint_stats_log_file_();

		std::string compile_log_file_name_(std::string const& file_suffix, std::string const& token_to_add);


		void start_render_loop(initial_rendering_mode_settings_t const& initial_render_mode_settings);


		void load_volumes(std::string const& volume_resource_string, glm::ivec3 const& persistent_extraction_block_size = glm::ivec3(0, 0, 0));

		void create_shader_programs();

	private:

		// imgui related
		void trigger_imgui_frame_();

	

		glm::uvec2 window_size_;

		AppRunDescriptor app_descriptor_;

		static bool draw_gui_;


		static std::unique_ptr<framework::BaseRenderer> renderer_;
	};


#endif //ISOSURFACE_RENDERER_APP