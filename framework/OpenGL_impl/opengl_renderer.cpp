#include "opengl_renderer.hpp"

#include "opengl_context.hpp"


#include "../glsl/shader_includes/atomic_counter_buffer_config.h.glsl"
#include "../glsl/shader_includes/per_frame_indirect_draw_parameter_ssbo_config.h.glsl"

#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// external libraries: GL and GL-Math related
#include <GL/gl3w.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>


#include "meshoptimizer.h"

#include <chrono>
#include <cassert>


extern uint16_t edgeTable[256];

extern uint8_t numTrisTable[256];
extern uint8_t numUniqueVertsTable[256];
extern uint8_t uniqueTriTable[256][16];

extern uint16_t constTriTablePacked[256][5];

namespace framework {

	OpenGLRenderer::OpenGLRenderer(int32_t preconfigured_index_buffer_size_in_MB, int32_t preconfigured_position_buffer_size_in_MB, int32_t preconfigured_normal_buffer_size_in_MB) 
		: BaseRenderer(preconfigured_index_buffer_size_in_MB, preconfigured_position_buffer_size_in_MB, preconfigured_normal_buffer_size_in_MB) {

	}

	void OpenGLRenderer::initialize_context_(glm::ivec2 const& requested_window_resolution) {
		assert(nullptr == opengl_context_);
		opengl_context_ = std::make_shared<framework::OpenGLGraphicsContext>(requested_window_resolution);
		base_context_ = opengl_context_;
	}

	void OpenGLRenderer::initialize_final_frame_buffers_() {
		assert(0 == draw_FBO_);
		// create custom FBO to become a layered frame buffer after adding layered attachments to it
		//glGenFramebuffers(1, &draw_FBO_);
		glCreateFramebuffers(1, &draw_FBO_);
		glBindFramebuffer(GL_FRAMEBUFFER, draw_FBO_);

		assert(nullptr == gpu_opengl_fbo_bindless_texture_addresses_buffer_);
		gpu_opengl_fbo_bindless_texture_addresses_buffer_ = std::make_unique<gpu_opengl_fbo_bindless_texture_adresses_buffer>();

		{
			assert(0 == draw_buffer_color_attachment_);
			// create layered color attachment containing 2 layers
			glGenTextures(1, &draw_buffer_color_attachment_);
			glBindTexture(GL_TEXTURE_2D_ARRAY, draw_buffer_color_attachment_);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB32F, window_resolution_[0], window_resolution_[1], 2, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);


			//glRenderbufferStorage()
			// set color texture sampling parameters
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


			glGenerateTextureMipmap(draw_buffer_color_attachment_);

			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);



			// attach it to currently bound framebuffer object
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, draw_buffer_color_attachment_, 0);

			
			{
				// get the resident handle for using the color texture as bindless texture -- used for sampling from it in final blit pass
				uint64_t resident_handle_color_texture_fbo = uint32_t(opengl_context_->loaded_api_extensions.glGetTextureHandleARB(draw_buffer_color_attachment_));

				// write the address into the sampler UBO CPU struct
				gpu_opengl_fbo_bindless_texture_addresses_buffer_->cpu_type_memory.resident_handle_color_texture_fbo
					= glm::uvec2(resident_handle_color_texture_fbo & 0xFFFFFFFF, resident_handle_color_texture_fbo >> 32);

				// make layered color texture resident
				opengl_context_->loaded_api_extensions.glMakeTextureHandleResidentARB(resident_handle_color_texture_fbo);
				if (!opengl_context_->loaded_api_extensions.glIsTextureHandleResidentARB(resident_handle_color_texture_fbo)) {
					std::cout << "Could not make color texture handle resident!" << std::endl;
					std::cout << "Exiting program." << std::endl;
					exit(-1);
				}
			}
			

		}

		assert(0 == draw_buffer_depth_attachment_);
		// create layered depth attachment containing 2 layers
		glGenTextures(1, &draw_buffer_depth_attachment_);
		glBindTexture(GL_TEXTURE_2D_ARRAY, draw_buffer_depth_attachment_);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, window_resolution_[0], window_resolution_[1], 2, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

		// set depth texture sampling parameters
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenerateTextureMipmap(draw_buffer_depth_attachment_);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		// attach the layered depth texture it to currently bound framebuffer object
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, draw_buffer_depth_attachment_, 0);

		// get the resident handle for using the depth texture as bindless texture -- only used for debugging visualizations
		uint64_t resident_handle_depth_texture_fbo = opengl_context_->loaded_api_extensions.glGetTextureHandleARB(draw_buffer_depth_attachment_);

		
		gpu_opengl_fbo_bindless_texture_addresses_buffer_->cpu_type_memory.resident_handle_depth_texture_fbo
			= glm::uvec2(resident_handle_depth_texture_fbo & 0xFFFFFFFF, resident_handle_depth_texture_fbo >> 32);
		


		// make layered depth texture resident
		opengl_context_->loaded_api_extensions.glMakeTextureHandleResidentARB(resident_handle_depth_texture_fbo);
		if (!opengl_context_->loaded_api_extensions.glIsTextureHandleResidentARB(resident_handle_depth_texture_fbo)) {
			std::cout << "Could not make depth texture handle resident!" << std::endl;
			std::cout << "Exiting program." << std::endl;
			exit(-1);
		}


		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
			std::cout << "Exiting program." << std::endl;
			
			exit(-1);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// using the CPU struct, we create and bind our texture sampler UBO
		
		
		glCreateBuffers(1, &gpu_opengl_fbo_bindless_texture_addresses_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_fbo_bindless_texture_addresses_buffer_->gl_buffer, sizeof(gpu_opengl_fbo_bindless_texture_addresses_buffer_->cpu_type_memory), (char*)(&gpu_opengl_fbo_bindless_texture_addresses_buffer_->cpu_type_memory), GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_fbo_bindless_texture_addresses_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_fbo_bindless_texture_addresses_buffer_->gl_buffer);
	}


	void OpenGLRenderer::register_volume_dataset(std::string const& resource_path, glm::ivec3 const& persistent_extraction_block_size) {

		
		auto registered_volume_pointer = std::make_shared<framework::OpenGLVolume>(opengl_context_, resource_path);

		add_volume_dataset_(registered_volume_pointer);
	}


	void OpenGLRenderer::operate_pre_frame() {
		BaseRenderer::operate_pre_frame();


		if (timer_mode::GLOBAL_FRAME == per_frame_performance_stats_.renderer_timer_mode) {
			complete_frame_without_gui_rendering_timer_->start();
		}

		if (needs_clear_color_update_) {
			glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], clear_color_[3]);
		}
		//clear default FBO
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	}

	void OpenGLRenderer::initialize_culling_reference_camera_buffer_() {
		gpu_opengl_culling_reference_camera_buffer_ = std::make_unique<gpu_opengl_culling_reference_camera_buffer>();

		BaseRenderer::initialize_culling_reference_camera_buffer_cpu_memory_(gpu_opengl_culling_reference_camera_buffer_->cpu_type_memory);

		glCreateBuffers(1, &gpu_opengl_culling_reference_camera_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_culling_reference_camera_buffer_->gl_buffer, sizeof(gpu_opengl_culling_reference_camera_buffer_->cpu_type_memory), (char*)(&gpu_opengl_culling_reference_camera_buffer_->cpu_type_memory), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_culling_reference_camera_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_culling_reference_camera_buffer_->gl_buffer);
	}

	void OpenGLRenderer::update_culling_reference_camera_buffer_() {
		BaseRenderer::update_culling_reference_camera_buffer_cpu_memory_();

		if (gpu_opengl_culling_reference_camera_buffer_->cpu_type_memory.is_dirty) {
			glNamedBufferSubData(gpu_opengl_culling_reference_camera_buffer_->gl_buffer, 0, sizeof(gpu_opengl_culling_reference_camera_buffer_->cpu_type_memory), (char*)(&gpu_opengl_culling_reference_camera_buffer_->cpu_type_memory));
			gpu_opengl_culling_reference_camera_buffer_->cpu_type_memory.is_dirty = false;
		}
	}

	void OpenGLRenderer::initialize_point_of_view_camera_buffer_() {
		gpu_opengl_point_of_view_camera_buffer_ = std::make_unique<gpu_opengl_point_of_view_camera_buffer>();

		BaseRenderer::initialize_point_of_view_camera_buffer_cpu_memory_(gpu_opengl_point_of_view_camera_buffer_->cpu_type_memory);

		glCreateBuffers(1, &gpu_opengl_point_of_view_camera_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_point_of_view_camera_buffer_->gl_buffer, sizeof(gpu_opengl_point_of_view_camera_buffer_->cpu_type_memory), (char*)(&gpu_opengl_point_of_view_camera_buffer_->cpu_type_memory), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_point_of_view_camera_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_point_of_view_camera_buffer_->gl_buffer);
	}


	void OpenGLRenderer::update_point_of_view_camera_buffer_() {
		BaseRenderer::update_point_of_view_camera_buffer_cpu_memory_();

		if (gpu_opengl_point_of_view_camera_buffer_->cpu_type_memory.is_dirty) {
			glNamedBufferSubData(gpu_opengl_point_of_view_camera_buffer_->gl_buffer, 0, sizeof(gpu_opengl_point_of_view_camera_buffer_->cpu_type_memory), (char*)(&gpu_opengl_point_of_view_camera_buffer_->cpu_type_memory));
			gpu_opengl_point_of_view_camera_buffer_->cpu_type_memory.is_dirty = false;

		}
	}

	void OpenGLRenderer::initialize_model_buffer_() {
		gpu_opengl_model_buffer_ = std::make_unique<gpu_opengl_model_buffer>();

		BaseRenderer::initialize_model_buffer_cpu_memory_(gpu_opengl_model_buffer_->cpu_type_memory);

		glCreateBuffers(1, &gpu_opengl_model_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_model_buffer_->gl_buffer, sizeof(gpu_opengl_model_buffer_->cpu_type_memory), (char*)(&gpu_opengl_model_buffer_->cpu_type_memory), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_model_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_model_buffer_->gl_buffer);
	}

	void OpenGLRenderer::update_model_buffer_() {
		BaseRenderer::update_model_buffer_cpu_memory_();

		if (gpu_opengl_model_buffer_->cpu_type_memory.is_dirty) {
			glNamedBufferSubData(gpu_opengl_model_buffer_->gl_buffer, 0, sizeof(gpu_opengl_model_buffer_->cpu_type_memory), (char*)(&gpu_opengl_model_buffer_->cpu_type_memory));
			gpu_opengl_model_buffer_->cpu_type_memory.is_dirty = false;
		}
	}


	void OpenGLRenderer::initialize_combined_model_culling_reference_camera_buffer_() {
		gpu_opengl_combined_model_culling_reference_camera_buffer_ = std::make_unique<gpu_opengl_combined_model_culling_reference_camera_buffer>();

		BaseRenderer::initialize_combined_model_culling_reference_camera_buffer_cpu_memory_(gpu_opengl_combined_model_culling_reference_camera_buffer_->cpu_type_memory);

		glCreateBuffers(1, &gpu_opengl_combined_model_culling_reference_camera_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_combined_model_culling_reference_camera_buffer_->gl_buffer, sizeof(gpu_opengl_combined_model_culling_reference_camera_buffer_->cpu_type_memory), (char*)(&gpu_opengl_combined_model_culling_reference_camera_buffer_->cpu_type_memory), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_combined_model_culling_reference_camera_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_combined_model_culling_reference_camera_buffer_->gl_buffer);
	}

	void OpenGLRenderer::update_combined_model_culling_reference_camera_buffer_() {
		BaseRenderer::update_combined_model_culling_reference_camera_buffer_cpu_memory_();

		if (gpu_opengl_combined_model_culling_reference_camera_buffer_->cpu_type_memory.is_dirty) {
			glNamedBufferSubData(gpu_opengl_combined_model_culling_reference_camera_buffer_->gl_buffer, 0, sizeof(gpu_opengl_combined_model_culling_reference_camera_buffer_->cpu_type_memory), (char*)(&gpu_opengl_combined_model_culling_reference_camera_buffer_->cpu_type_memory));
			gpu_opengl_combined_model_culling_reference_camera_buffer_->cpu_type_memory.is_dirty = false;
		}
	}

	void OpenGLRenderer::initialize_combined_model_point_of_view_camera_buffer_() {
		gpu_opengl_combined_model_point_of_view_camera_buffer_ = std::make_unique<gpu_opengl_combined_model_point_of_view_camera_buffer>();

		BaseRenderer::initialize_combined_model_point_of_view_camera_buffer_cpu_memory_(gpu_opengl_combined_model_point_of_view_camera_buffer_->cpu_type_memory);

		glCreateBuffers(1, &gpu_opengl_combined_model_point_of_view_camera_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_combined_model_point_of_view_camera_buffer_->gl_buffer, sizeof(gpu_opengl_combined_model_point_of_view_camera_buffer_->cpu_type_memory), (char*)(&gpu_opengl_combined_model_point_of_view_camera_buffer_->cpu_type_memory), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_combined_model_point_of_view_camera_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_combined_model_point_of_view_camera_buffer_->gl_buffer);
	}

	void OpenGLRenderer::update_combined_model_point_of_view_camera_buffer_() {
		BaseRenderer::update_combined_model_point_of_view_camera_buffer_cpu_memory_();
		
		
		if (gpu_opengl_combined_model_point_of_view_camera_buffer_->cpu_type_memory.is_dirty) {
			glNamedBufferSubData(gpu_opengl_combined_model_point_of_view_camera_buffer_->gl_buffer, 0, sizeof(gpu_opengl_combined_model_point_of_view_camera_buffer_->cpu_type_memory), (char*)(&gpu_opengl_combined_model_point_of_view_camera_buffer_->cpu_type_memory));
			gpu_opengl_combined_model_point_of_view_camera_buffer_->cpu_type_memory.is_dirty = false;	
		}
	}



	void OpenGLRenderer::initialize_draw_parameters_buffer_() {
		gpu_opengl_draw_parameters_buffer_ = std::make_unique<gpu_opengl_draw_parameters_buffer>();

		BaseRenderer::initialize_draw_parameters_buffer_(gpu_opengl_draw_parameters_buffer_->cpu_type_memory);

		glCreateBuffers(1, &gpu_opengl_draw_parameters_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_draw_parameters_buffer_->gl_buffer, sizeof(gpu_opengl_draw_parameters_buffer_->cpu_type_memory), (char*)(&gpu_opengl_draw_parameters_buffer_->cpu_type_memory), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_draw_parameters_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_draw_parameters_buffer_->gl_buffer);
	}

	void OpenGLRenderer::update_draw_parameters_buffer_() {
		BaseRenderer::update_draw_parameters_buffer_cpu_memory_();

		if (gpu_opengl_draw_parameters_buffer_->cpu_type_memory.is_dirty) {
			glNamedBufferSubData(gpu_opengl_draw_parameters_buffer_->gl_buffer, 0, sizeof(gpu_opengl_draw_parameters_buffer_->cpu_type_memory), (char*)(&gpu_opengl_draw_parameters_buffer_->cpu_type_memory));
			gpu_opengl_draw_parameters_buffer_->cpu_type_memory.is_dirty = false;
		}
	}


	void OpenGLRenderer::initialize_extraction_parameters_buffer_() {
		gpu_opengl_extraction_parameters_buffer_ = std::make_unique<gpu_opengl_extraction_parameters_buffer>();

		BaseRenderer::initialize_extraction_parameters_buffer_(gpu_opengl_extraction_parameters_buffer_->cpu_type_memory);

		glCreateBuffers(1, &gpu_opengl_extraction_parameters_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_extraction_parameters_buffer_->gl_buffer, sizeof(gpu_opengl_extraction_parameters_buffer_->cpu_type_memory), (char*)(&gpu_opengl_extraction_parameters_buffer_->cpu_type_memory), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_extraction_parameters_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_extraction_parameters_buffer_->gl_buffer);
	}

	void OpenGLRenderer::update_extraction_parameters_buffer_() {
		BaseRenderer::update_extraction_parameters_buffer_cpu_memory_();

		if (gpu_opengl_extraction_parameters_buffer_->cpu_type_memory.is_dirty) {
			glNamedBufferSubData(gpu_opengl_extraction_parameters_buffer_->gl_buffer, 0, sizeof(gpu_opengl_extraction_parameters_buffer_->cpu_type_memory), (char*)(&gpu_opengl_extraction_parameters_buffer_->cpu_type_memory));
			gpu_opengl_extraction_parameters_buffer_->cpu_type_memory.is_dirty = false;
		}
	}

	void OpenGLRenderer::initialize_render_frame_descriptor_buffer_() {
		gpu_opengl_render_frame_descriptor_buffer_ = std::make_unique<gpu_opengl_render_frame_descriptor_buffer>();
		BaseRenderer::initialize_render_frame_descriptor_buffer_(gpu_opengl_render_frame_descriptor_buffer_->cpu_type_memory);

		glCreateBuffers(1, &gpu_opengl_render_frame_descriptor_buffer_->gl_buffer);
		glNamedBufferData(gpu_opengl_render_frame_descriptor_buffer_->gl_buffer, sizeof(gpu_opengl_render_frame_descriptor_buffer_->cpu_type_memory), (char*)(&gpu_opengl_render_frame_descriptor_buffer_->cpu_type_memory), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, gpu_opengl_render_frame_descriptor_buffer_->cpu_type_memory.UBO_BINDING_POINT, gpu_opengl_render_frame_descriptor_buffer_->gl_buffer);

	}

	void OpenGLRenderer::update_render_frame_descriptor_buffer_() {
		BaseRenderer::update_render_frame_descriptor_buffer_cpu_memory_();

		if (gpu_opengl_render_frame_descriptor_buffer_->cpu_type_memory.is_dirty) {
			glNamedBufferSubData(gpu_opengl_render_frame_descriptor_buffer_->gl_buffer, 0, sizeof(gpu_opengl_render_frame_descriptor_buffer_->cpu_type_memory), (char*)(&gpu_opengl_render_frame_descriptor_buffer_->cpu_type_memory));
			gpu_opengl_render_frame_descriptor_buffer_->cpu_type_memory.is_dirty = false;
		}
	}


	camera_descriptor_t OpenGLRenderer::create_default_camera_descriptor_(glm::ivec2 const& window_resolution, glm::vec3 const& culling_camera_reference_position) const {
		return framework::camera_descriptor_t{ 30.0f, window_resolution, {0.05f, 20.0f }, culling_camera_reference_position, glm::vec3(0.0f, 1.0f, 0.0f), true};
	}


	void OpenGLRenderer::initialize_atomic_counters_() {}

	void OpenGLRenderer::update_UBOs_() {
		//update_view_buffer_();
		update_culling_reference_camera_buffer_();
		update_point_of_view_camera_buffer_();
		update_model_buffer_();
		update_combined_model_culling_reference_camera_buffer_();
		update_combined_model_point_of_view_camera_buffer_();

		update_draw_parameters_buffer_();
		update_extraction_parameters_buffer_();
		update_render_frame_descriptor_buffer_();

	}

	void OpenGLRenderer::initialize_UBOs_() {
		if (nullptr != culling_reference_camera_) {
			initialize_culling_reference_camera_buffer_();
			initialize_point_of_view_camera_buffer_();
			initialize_model_buffer_();
			initialize_combined_model_culling_reference_camera_buffer_();
			initialize_combined_model_point_of_view_camera_buffer_();
			initialize_draw_parameters_buffer_();
			initialize_extraction_parameters_buffer_();
			initialize_render_frame_descriptor_buffer_();
		}
	}

	void OpenGLRenderer::operate_post_frame() {
		BaseRenderer::operate_post_frame();

		//collect timing measurements
		switch (per_frame_performance_stats_.renderer_timer_mode) {
			case timer_mode::GLOBAL_FRAME: {
				complete_frame_without_gui_rendering_timer_->end_wait_and_evaluate();
				per_frame_performance_stats_.measured_time.global_time_ms = complete_frame_without_gui_rendering_timer_->get_elapsed_milliseconds();
				break;
			}
				
			case timer_mode::PER_PIPELINE_COMPONENT: {
				per_component_timers__draw_times_aux_vis_draw_timer_->wait_for_query_result(); // first wait for the last timer in our sequence
				per_component_timers__draw_times_aux_vis_draw_timer_->evaluate_elapsed_time(); // first wait for the last timer in our sequence
				per_component_timers__draw_times_isosurface_draw_timer_->evaluate_elapsed_time(); // now also fetch the dependent isosurface draw time

				per_component_timers__extraction_times_isosurface_extraction_timer_->evaluate_elapsed_time();
				per_component_timers__dense_occupancy_computation_timer_->evaluate_elapsed_time();

				per_frame_performance_stats_.measured_time.per_component.dense_occupancy_computation_time_ms = per_component_timers__dense_occupancy_computation_timer_->get_elapsed_milliseconds();
				per_frame_performance_stats_.measured_time.per_component.pure_extraction_time_ms = per_component_timers__extraction_times_isosurface_extraction_timer_->get_elapsed_milliseconds();
				
				per_frame_performance_stats_.measured_time.per_component.pure_isosurface_rendering_time_ms = per_component_timers__draw_times_isosurface_draw_timer_->get_elapsed_milliseconds();
				per_frame_performance_stats_.measured_time.per_component.pure_auxiliary_vis_rendering_time_ms = per_component_timers__draw_times_aux_vis_draw_timer_->get_elapsed_milliseconds();
				per_frame_performance_stats_.measured_time.per_component.accumulated_frame_time_ms
					=
					per_frame_performance_stats_.measured_time.per_component.dense_occupancy_computation_time_ms
					+ per_frame_performance_stats_.measured_time.per_component.pure_extraction_time_ms
					+ per_frame_performance_stats_.measured_time.per_component.pure_isosurface_rendering_time_ms
					+ per_frame_performance_stats_.measured_time.per_component.pure_auxiliary_vis_rendering_time_ms;
				break;
			}

			default:
				std::cout << "Unknown timer mode" << std::endl;
		}
		

	}

	void OpenGLRenderer::finalize_frame() {
		opengl_context_->swap_buffer_and_poll_events();
	}

	void OpenGLRenderer::register_gui() {
		opengl_context_->register_gui();
	}

	void OpenGLRenderer::start_gui_frame() {
		opengl_context_->start_gui_frame();

		BaseRenderer::start_gui_frame();
	}
	
	void OpenGLRenderer::end_gui_frame() {
		opengl_context_->end_gui_frame();
	};

	void OpenGLRenderer::start_isosurface_draw_timer_() {
		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__draw_times_isosurface_draw_timer_->start();
		}
	}
	void OpenGLRenderer::end_isosurface_draw_timer_() {
		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__draw_times_isosurface_draw_timer_->end();
		}
	}

	void OpenGLRenderer::start_auxiliary_visualization_draw_timer_() {
		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__draw_times_aux_vis_draw_timer_->start();
		}
	}
	void OpenGLRenderer::end_auxiliary_visualization_draw_timer_() {
		if (timer_mode::PER_PIPELINE_COMPONENT == per_frame_performance_stats_.renderer_timer_mode) {
			per_component_timers__draw_times_aux_vis_draw_timer_->end();
		}
	}

	void OpenGLRenderer::render_volume_boundaries_() {
		glLineWidth(3);
		//pull line vis slightly in front of actual subvolume depth values to avoid flickering aux vis
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1, -1);

		framework_shader_programs_.aux_vis.volume_boundary_vis_program_ptr->use();

		opengl_context_->loaded_api_extensions.glDrawMeshTasksNV(0, 1);

		glLineWidth(1);
		glPolygonOffset(0, 0);
		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void OpenGLRenderer::render_culling_reference_camera_frustum_() {
		framework_shader_programs_.aux_vis.render_culling_camera_frustum_program_ptr->use();
		glLineWidth(3);
		opengl_context_->loaded_api_extensions.glDrawMeshTasksNV(0, 1);
		glLineWidth(1);
	}

	void OpenGLRenderer::render_coordinate_axes_visualization_() {
		framework_shader_programs_.aux_vis.coordinate_system_axes_vis_program_ptr->use();
		glLineWidth(3);
		glDrawArrays(GL_LINES, 0, 6);
		glLineWidth(1);
	}

	std::unique_ptr<BaseGPUTimer> OpenGLRenderer::create_new_gpu_timer_instance_(std::string const& timer_name) const {
		return std::make_unique<OpenGLGPUTimer>(timer_name);
	}

	void OpenGLRenderer::release_gpu_resources_() {
	}

	void OpenGLRenderer::allocate_storage_buffers_() {
		auto const& descriptor_ = *main_volume_descriptor_;

		{
			uint64_t const max_num_indices_vbo
				= static_cast<uint64_t>(descriptor_.resolution[0]) * descriptor_.resolution[1] * descriptor_.resolution[2] / 2;// *50000000u;
			uint64_t const max_num_vertices_vbo = max_num_indices_vbo / 3;
			uint64_t const num_bytes_per_position = 4 * sizeof(float);
			num_bytes_to_conservatively_allocate_pos_VBO_ = (preconfigured_position_buffer_size_in_MB_ == -1) ? max_num_vertices_vbo * num_bytes_per_position : (static_cast<uint64_t>(preconfigured_position_buffer_size_in_MB_) * (1024llu * 1024llu));
			uint64_t const num_bytes_per_normal = 4 * sizeof(float);
			num_bytes_to_conservatively_allocate_nor_VBO_ = (preconfigured_normal_buffer_size_in_MB_ == -1) ? max_num_vertices_vbo * num_bytes_per_normal : (static_cast<uint64_t>(preconfigured_normal_buffer_size_in_MB_) * (1024llu * 1024llu));

			uint64_t const num_bytes_per_index = 4 *  sizeof(uint8_t);//sizeof(uint8_t); //allocate with meshlets in mind
			num_bytes_to_conservatively_allocate_EBO__32bit_indices = (preconfigured_index_buffer_size_in_MB_ == -1) ? (max_num_indices_vbo * num_bytes_per_index) : (static_cast<uint64_t>(preconfigured_index_buffer_size_in_MB_) * (1024llu * 1024llu));



			remapped_indices_.resize(num_bytes_to_conservatively_allocate_EBO__32bit_indices / sizeof(uint32_t));
			index_remap_table_.resize(num_bytes_to_conservatively_allocate_EBO__32bit_indices / sizeof(uint32_t));

			positions_cpu_.resize(num_bytes_to_conservatively_allocate_pos_VBO_ / sizeof(glm::vec3));
			normals_cpu_.resize(num_bytes_to_conservatively_allocate_nor_VBO_ / sizeof(glm::vec3));
			indices_cpu_.resize(num_bytes_to_conservatively_allocate_EBO__32bit_indices / sizeof(uint32_t));


			uint64_t total_num_voxels
				= static_cast<uint64_t>(main_volume_descriptor_->resolution[0]) *
				static_cast<uint64_t>(main_volume_descriptor_->resolution[1]) *
				static_cast<uint64_t>(main_volume_descriptor_->resolution[2]);

			uint64_t constexpr conservatively_allocated_num_meshlets = 5'000'000u;
			built_meshlets_cpu_.resize(conservatively_allocated_num_meshlets);

			//MESHLET related
			{

				num_bytes_to_conservatively_allocate_meshlet_vertex_offset_and_count__index_offset_and_count__buffer_ = conservatively_allocated_num_meshlets * 4u * sizeof(uint32_t);
				glCreateBuffers(1, &persistent_extraction_meshlet_vertex_offset_and_count__index_offset_and_count__buffer_);
				glNamedBufferData(persistent_extraction_meshlet_vertex_offset_and_count__index_offset_and_count__buffer_, num_bytes_to_conservatively_allocate_meshlet_vertex_offset_and_count__index_offset_and_count__buffer_, nullptr, GL_DYNAMIC_COPY);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 18, persistent_extraction_meshlet_vertex_offset_and_count__index_offset_and_count__buffer_);
			}


			{
				glCreateVertexArrays(1, &VAO_solid_SOA);

				{
					uint32_t constexpr NORMAL_ATTRIB_INDEX = 0;
					glEnableVertexArrayAttrib(VAO_solid_SOA, NORMAL_ATTRIB_INDEX);
					glVertexArrayVertexBuffer(VAO_solid_SOA, NORMAL_ATTRIB_INDEX, persistent_extraction_nor_VBO_, 0, sizeof(glm::vec3));
					glVertexArrayAttribFormat(VAO_solid_SOA, NORMAL_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
					glVertexArrayAttribBinding(VAO_solid_SOA, NORMAL_ATTRIB_INDEX, 0);
				}
				
			}

			glCreateBuffers(1, &persistent_extraction_pos_VBO_);
			glNamedBufferData(persistent_extraction_pos_VBO_, num_bytes_to_conservatively_allocate_pos_VBO_, nullptr, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, persistent_extraction_pos_VBO_);

			glCreateBuffers(1, &persistent_extraction_nor_VBO_);
			glNamedBufferData(persistent_extraction_nor_VBO_, num_bytes_to_conservatively_allocate_nor_VBO_, nullptr, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, persistent_extraction_nor_VBO_);



			{
				glCreateBuffers(1, &persistent_extraction_EBO__indices);
				glCreateVertexArrays(1, &VAO_solid_SOA_with_index_buffer__position_and_normal);


				glNamedBufferData(persistent_extraction_EBO__indices, num_bytes_to_conservatively_allocate_EBO__32bit_indices, nullptr, GL_DYNAMIC_COPY);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, persistent_extraction_EBO__indices);

				glVertexArrayElementBuffer(VAO_solid_SOA_with_index_buffer__position_and_normal, persistent_extraction_EBO__indices);

				{

					uint32_t constexpr NORMAL_ATTRIB_INDEX = 0;
					glEnableVertexArrayAttrib(VAO_solid_SOA_with_index_buffer__position_and_normal, NORMAL_ATTRIB_INDEX);
					glVertexArrayVertexBuffer(VAO_solid_SOA_with_index_buffer__position_and_normal, NORMAL_ATTRIB_INDEX, persistent_extraction_nor_VBO_, 0, sizeof(glm::vec3));
					glVertexArrayAttribFormat(VAO_solid_SOA_with_index_buffer__position_and_normal, NORMAL_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
					glVertexArrayAttribBinding(VAO_solid_SOA_with_index_buffer__position_and_normal, NORMAL_ATTRIB_INDEX, 0);

				}
			}

			//this buffer is only really used in a mode where we compare our approach to meshoptimizer, since we need this additional storage there.
			//for efficiency and memory footprint purposes, we may be better off only allocating this lazily
			{
				num_bytes_meshlet_vertex_indices_buffer_32_bit_ = num_bytes_to_conservatively_allocate_EBO__32bit_indices;
				glCreateBuffers(1, &meshoptimizer_created_meshlet_vertex_indices_buffer_32bit_);

				glNamedBufferData(meshoptimizer_created_meshlet_vertex_indices_buffer_32bit_, num_bytes_to_conservatively_allocate_EBO__32bit_indices, nullptr, GL_DYNAMIC_COPY);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, meshoptimizer_created_meshlet_vertex_indices_buffer_32bit_);


			}

			num_bytes_meshlet_culling_info_ = conservatively_allocated_num_meshlets * sizeof(glm::vec4);

			glCreateBuffers(1, &meshlet_culling_info_buffer_);
			glNamedBufferData(meshlet_culling_info_buffer_, num_bytes_meshlet_culling_info_, nullptr, GL_DYNAMIC_COPY);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, meshlet_culling_info_buffer_);
		}
	}

	void OpenGLRenderer::allocate_shader_storage_buffer_objects_() {
		assert(nullptr != main_volume_descriptor_);
		auto& descriptor_ = *main_volume_descriptor_;



		uint32_t const num_blocks_in_volume
			= uint32_t(std::ceil(descriptor_.resolution[0] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[0])))
			* uint32_t(std::ceil(descriptor_.resolution[1] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[1])))
			* uint32_t(std::ceil(descriptor_.resolution[2] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[2])));

		uint32_t const estimation_max_num_occupied_cells = descriptor_.resolution[0] * descriptor_.resolution[1] * descriptor_.resolution[2];



		num_bytes_occupied_block_indices_ssbo_ = num_blocks_in_volume * sizeof(uint32_t);
		glCreateBuffers(1, &occupied_block_indices_ssbo_);
		glNamedBufferData(occupied_block_indices_ssbo_, num_bytes_occupied_block_indices_ssbo_, 0x0, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 40, occupied_block_indices_ssbo_);
		
		
		num_bytes_dense_meshlet_group_offset_and_length_ssbo_ = num_blocks_in_volume * sizeof(uint32_t);
		glCreateBuffers(1, &dense_meshlet_group_offset_and_length_ssbo_);
		glNamedBufferData(dense_meshlet_group_offset_and_length_ssbo_, num_bytes_dense_meshlet_group_offset_and_length_ssbo_, 0x0, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 41, dense_meshlet_group_offset_and_length_ssbo_);


		glCreateBuffers(1, &indirect_compute_buffer);
		//for now we use slot 0 for the coarse-fine occupancy test
		glm::uvec3 default_dispatch_args = { 1, 1, 1 };
		glNamedBufferData(indirect_compute_buffer, sizeof(glm::uvec3), glm::value_ptr(default_dispatch_args), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, indirect_compute_buffer);
		
		// max number of 8x8x8 voxels
		descriptor_.num_max_compute_shader_cull_groups = glm::ivec3(descriptor_.resolution) / glm::ivec3(8, 8, 8);

		uint32_t num_indices_to_allocate_in_ssbo
			= int64_t(descriptor_.resolution[0] / general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[0])
			* int64_t(descriptor_.resolution[1] / general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[1])
			* int64_t(descriptor_.resolution[2] / general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[2])
			;

		overall_num_blocks_in_volume_ = num_indices_to_allocate_in_ssbo;

		descriptor_.num_ssbo_elements_to_clear = num_indices_to_allocate_in_ssbo;

		///////////////////////////////////////////

		uint32_t const BLOCK_PATH_DRAW_CALL_HEADER_BYTES = sizeof(uint32_t) * NUM_BLOCK_BASED_DRAW_CALL_HEADER_SSBO_UINT32_SLOTS;
		uint32_t const NUM_INDIRECT_DRAW_COMMAND_COPIES_TO_MAINTAIN_IN_BLOCK_BASED_MODES = 3;

		uint32_t const MESHLET_PATH_DRAW_CALL_HEADER_BYTES = sizeof(uint32_t) * NUM_MESHLET_BASED_DRAW_CALL_HEADER_SSBO_UINT32_SLOTS;
		uint32_t const NUM_INDIRECT_DRAW_COMMAND_COPIES_TO_MAINTAIN_IN_MESHLET_BASED_MODES = 2;

		num_byte_per_frame_block_based_indirect_draw_parameter_ssbo_
			= BLOCK_PATH_DRAW_CALL_HEADER_BYTES + NUM_INDIRECT_DRAW_COMMAND_COPIES_TO_MAINTAIN_IN_BLOCK_BASED_MODES * overall_num_blocks_in_volume_ * sizeof(DrawElementsIndirectCommand) +// + sizeof(IndirectDrawStructTaskMeshCommand)
			  MESHLET_PATH_DRAW_CALL_HEADER_BYTES + NUM_INDIRECT_DRAW_COMMAND_COPIES_TO_MAINTAIN_IN_MESHLET_BASED_MODES * overall_num_blocks_in_volume_ * sizeof(uint32_t);
			;// +NUM_INDIRECT_DRAW_COMMAND_COPIES_TO_MAINTAIN_IN_MESHLET_BASED_MODES * overall_num_blocks_in_volume_ * sizeof(uint32_t);// +sizeof(IndirectDrawStructTaskMeshCommand);

		glCreateBuffers(1, &per_frame_block_based_indirect_draw_parameter_ssbo_);
		glNamedBufferStorage(per_frame_block_based_indirect_draw_parameter_ssbo_, num_byte_per_frame_block_based_indirect_draw_parameter_ssbo_, 0x0, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, per_frame_block_based_indirect_draw_parameter_ssbo_);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, per_frame_block_based_indirect_draw_parameter_ssbo_);
		currently_bound_indirect_draw_buffer = per_frame_block_based_indirect_draw_parameter_ssbo_;
	}


	void OpenGLRenderer::allocate_texture_buffer_objects_() {

		// marching cubes tables which will be used for marching cubes in the transient isosurface extraction
		// mesh shaders (tables based on https://github.com/tpn/cuda-samples/tree/master/v8.0/2_Graphics/marchingCubes)
		// and derivatives for unique vertex extraction per cell
		glCreateTextures(GL_TEXTURE_BUFFER, 1, &gl_edgeTable_buffer_tbo_);
		glCreateBuffers(1, &gl_edgeTable_buffer_ref_);
		glNamedBufferStorage(gl_edgeTable_buffer_ref_, sizeof(edgeTable), &edgeTable[0], 0x0);
		glTextureBuffer(gl_edgeTable_buffer_tbo_, GL_R16UI, gl_edgeTable_buffer_ref_);

		glActiveTexture(GL_TEXTURE0 + 31);
		glBindTexture(GL_TEXTURE_BUFFER, gl_edgeTable_buffer_tbo_);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, gl_edgeTable_buffer_ref_);

		glCreateTextures(GL_TEXTURE_BUFFER, 1, &gl_unique_vertices_triTable_buffer_tbo_);
		glCreateBuffers(1, &gl_unique_vertices_triTable_buffer_ref_);
		glNamedBufferStorage(gl_unique_vertices_triTable_buffer_ref_, sizeof(uniqueTriTable), &uniqueTriTable[0], 0x0);
		glTextureBuffer(gl_unique_vertices_triTable_buffer_tbo_, GL_R8UI, gl_unique_vertices_triTable_buffer_ref_);

		glActiveTexture(GL_TEXTURE0 + 32);
		glBindTexture(GL_TEXTURE_BUFFER, gl_unique_vertices_triTable_buffer_tbo_);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, gl_unique_vertices_triTable_buffer_ref_);

		glCreateTextures(GL_TEXTURE_BUFFER, 1, &gl_total_numTris_table_buffer_tbo_);
		glCreateBuffers(1, &gl_total_numTris_table_buffer_ref_);
		glNamedBufferStorage(gl_total_numTris_table_buffer_ref_, sizeof(numTrisTable), &numTrisTable[0], 0x0);
		glTextureBuffer(gl_total_numTris_table_buffer_tbo_, GL_R8UI, gl_total_numTris_table_buffer_ref_);

		glActiveTexture(GL_TEXTURE0 + 33);
		glBindTexture(GL_TEXTURE_BUFFER, gl_total_numTris_table_buffer_tbo_);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, gl_total_numTris_table_buffer_ref_);

		glCreateTextures(GL_TEXTURE_BUFFER, 1, &gl_unique_numVerts_table_buffer_tbo_);
		glCreateBuffers(1, &gl_unique_numVerts_table_buffer_ref_);
		glNamedBufferStorage(gl_unique_numVerts_table_buffer_ref_, sizeof(numUniqueVertsTable), &numUniqueVertsTable[0], 0x0);
		glTextureBuffer(gl_unique_numVerts_table_buffer_tbo_, GL_R8UI, gl_unique_numVerts_table_buffer_ref_);

		glActiveTexture(GL_TEXTURE0 + 34);
		glBindTexture(GL_TEXTURE_BUFFER, gl_unique_numVerts_table_buffer_tbo_);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, gl_unique_numVerts_table_buffer_ref_);


		glCreateTextures(GL_TEXTURE_BUFFER, 1, &gl_16_bit_packed_tri_table_buffer_tbo_);
		glCreateBuffers(1, &gl_16_bit_packed_tri_table_buffer_ref_);
		glNamedBufferStorage(gl_16_bit_packed_tri_table_buffer_ref_, sizeof(constTriTablePacked), &constTriTablePacked[0], 0x0);
		glTextureBuffer(gl_16_bit_packed_tri_table_buffer_tbo_, GL_R16UI, gl_16_bit_packed_tri_table_buffer_ref_);

		glActiveTexture(GL_TEXTURE0 + 35);
		glBindTexture(GL_TEXTURE_BUFFER, gl_16_bit_packed_tri_table_buffer_tbo_);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, gl_16_bit_packed_tri_table_buffer_ref_);

		uint32_t const atomic_counter_buffer_size = NUM_JOINT_ATOMIC_COUNTERS * sizeof(uint32_t);
		glCreateBuffers(1, &collapsed_atomic_counter_buffers);
		glNamedBufferData(collapsed_atomic_counter_buffers, atomic_counter_buffer_size, 0x0, GL_DYNAMIC_COPY);


		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, collapsed_atomic_counter_buffers);
		glBindBuffer(GL_PARAMETER_BUFFER, per_frame_block_based_indirect_draw_parameter_ssbo_);
		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, collapsed_atomic_counter_buffers);

	}

	void OpenGLRenderer::create_timer_query_objects_() {
	}



	void OpenGLRenderer::collect_extracted_geometry_statistics_() {
		if (memory_footprint_mode::NONE != per_frame_performance_stats_.renderer_memory_footprint_mode) {
			uint32_t num_extracted_blocks = 0u;
			glGetNamedBufferSubData(collapsed_atomic_counter_buffers, NUM_OCCUPIED_BLOCKS_ATOMIC_COUNTER_SSBO_UINT32_OFFSET * sizeof(uint32_t), sizeof(uint32_t), &num_extracted_blocks);
			uint32_t num_extracted_meshlets = 0u;
			glGetNamedBufferSubData(collapsed_atomic_counter_buffers, NUM_EXTRACTED_MESHLETS_TASK_MESH_SHADER_COUNTER_SSBO_UINT32_OFFSET * sizeof(uint32_t), sizeof(uint32_t), &num_extracted_meshlets);
			uint32_t num_extracted_vertices = 0u;
			glGetNamedBufferSubData(collapsed_atomic_counter_buffers, NUM_EXTRACTED_VERTICES_ATOMIC_COUNTER_SSBO_UINT32_OFFSET * sizeof(uint32_t), sizeof(uint32_t), &num_extracted_vertices);
			uint32_t num_extracted_triangles = 0u;
			glGetNamedBufferSubData(collapsed_atomic_counter_buffers, NUM_EXTRACTED_INDICES_ATOMIC_COUNTER_SSBO_UINT32_OFFSET * sizeof(uint32_t), sizeof(uint32_t), &num_extracted_triangles);

			uint32_t const num_byte_for_vertex_positions = sizeof(glm::vec3) * num_extracted_vertices ;
			uint32_t const num_byte_for_vertex_normals   = sizeof(glm::vec3) * num_extracted_vertices ;

			uint32_t num_byte_for_block_descriptor_persistent_part = 0u;
			uint32_t num_byte_for_block_descriptor_indirect_draw_call_part = 0u;

			uint32_t num_byte_for_meshlet_descriptors = 0u;


			uint32_t num_byte_for_culling_info = 0u;

			switch (renderer_conf.indirect_rendering_compute.compute_mode) {
			case IndirectRenderingComputeMode::COMPUTE_SHADER_BASED_EXTRACTION: {


				switch (renderer_conf.indirect_rendering_compute.compute_shader_path_extraction_mode) {
					case ComputeShaderExtractionMode::SINGLE_INDEX_ARRAY:
					{
						//do not add memory footprint for block based descriptors
						num_byte_for_block_descriptor_persistent_part = 0u;
						num_byte_for_block_descriptor_indirect_draw_call_part = 0u;
						num_byte_per_extracted_triangle_index_ = 4u;
						break;
					}

					case ComputeShaderExtractionMode::BLOCKWISE_32BIT_INDICES: {
						num_byte_per_extracted_triangle_index_ = 4u;
						num_byte_for_block_descriptor_persistent_part = num_extracted_blocks * 2u;
						num_byte_for_block_descriptor_indirect_draw_call_part = num_extracted_blocks * 3u;
						num_byte_for_culling_info = num_extracted_blocks * sizeof(uint32_t);
						break;
					}
					case ComputeShaderExtractionMode::BLOCKWISE_16BIT_INDICES: {
						num_byte_per_extracted_triangle_index_ = 2u;
						num_byte_for_block_descriptor_persistent_part = num_extracted_blocks * 2u;
						num_byte_for_block_descriptor_indirect_draw_call_part = num_extracted_blocks * 3u;
						num_byte_for_culling_info = num_extracted_blocks * sizeof(uint32_t);
						break;
					}

					default:
						std::cout << "Unknown compute shader based extraction mode" << std::endl;
						break;
				}
				break;
			}
			case IndirectRenderingComputeMode::TASK_MESH_SHADER_BASED_EXTRACTION: {
				num_byte_per_extracted_triangle_index_ = 1u;
				num_byte_for_meshlet_descriptors = num_extracted_meshlets * 4u * sizeof(uint32_t);;
				num_byte_for_culling_info = num_extracted_blocks * sizeof(uint32_t) * 2u;
				break;
			}

			case IndirectRenderingComputeMode::MESHOPT_OFFLINE_MESHLET_COMPUTATION: {

				num_byte_per_extracted_triangle_index_ = 1u;
				num_byte_for_meshlet_descriptors = num_extracted_meshlets * 4u * sizeof(uint32_t);;
				num_byte_for_culling_info = num_extracted_meshlets * 11u * sizeof(float);
				break;
			}
		
			default:
				std::cout << "Unknown extraction mode" << std::endl;
				exit(-1);
			}

			uint32_t const num_byte_for_triangles = num_byte_per_extracted_triangle_index_ * num_extracted_triangles * 3u;

			uint64_t const total_memory_footprint_in_byte
				= (IndirectRenderingComputeMode::MESHOPT_OFFLINE_MESHLET_COMPUTATION == renderer_conf.indirect_rendering_compute.compute_mode) ?
				
				num_byte_offline_built_meshlets_ : 
				
				num_byte_for_vertex_positions
				+ num_byte_for_vertex_normals
				+ num_byte_for_triangles
				+ num_byte_for_block_descriptor_persistent_part
				+ num_byte_for_block_descriptor_indirect_draw_call_part;


			switch (per_frame_performance_stats_.renderer_memory_footprint_mode) {
				case memory_footprint_mode::GLOBAL_FRAME: {
					per_frame_performance_stats_.measured_memory_footprints.total_amount_byte = total_memory_footprint_in_byte;
					break;
				}
				case memory_footprint_mode::PER_MEMORY_COMPONENT: {
					per_frame_performance_stats_.measured_memory_footprints.separated_memory_amounts_byte.indices = num_byte_for_triangles;
					per_frame_performance_stats_.measured_memory_footprints.separated_memory_amounts_byte.vertex_positions = num_byte_for_vertex_positions;
					per_frame_performance_stats_.measured_memory_footprints.separated_memory_amounts_byte.vertex_normals = num_byte_for_vertex_normals;

					per_frame_performance_stats_.measured_memory_footprints.separated_memory_amounts_byte.block_descriptors_persistent_part = num_byte_for_block_descriptor_persistent_part;
					per_frame_performance_stats_.measured_memory_footprints.separated_memory_amounts_byte.block_descriptors_live_part = num_byte_for_block_descriptor_indirect_draw_call_part;
					per_frame_performance_stats_.measured_memory_footprints.separated_memory_amounts_byte.meshlet_descriptors = num_byte_for_meshlet_descriptors;
					per_frame_performance_stats_.measured_memory_footprints.separated_memory_amounts_byte.culling_information = num_byte_for_culling_info;

					per_frame_performance_stats_.measured_memory_footprints.separated_memory_amounts_byte.total = total_memory_footprint_in_byte;

					break;
				}
			}

			std::cout << "Memory footprint: " << total_memory_footprint_in_byte / static_cast<float>(1 << 20) << " MB (" << total_memory_footprint_in_byte << ")" << std::endl;
		}
	}

	void OpenGLRenderer::hotplug_new_vertex_and_triangle_counts_for_indirect_meshlet_rendering_mode() {
		latest_hotplug_request_
			= { "#undef MAX_VERTS", "#undef MAX_TRIS",
			   "#define MAX_VERTS " + std::to_string(get_max_num_vertices_for_forward_meshlet_rendering()),
			   "#define MAX_TRIS " + std::to_string(get_max_num_triangles_for_forward_meshlet_rendering()) };
	}


	float OpenGLRenderer::retrieve_min_max_grid_computation_time_ms_() {
		uint64_t min_max_computation_timer_result_start = 0;
		uint64_t min_max_computation_timer_result_end = 0;
	
		return 1e-6 * (min_max_computation_timer_result_end - min_max_computation_timer_result_start);
	}

	float OpenGLRenderer::retrieve_dense_occupancy_brick_indices_computation_time_ms_() {
		uint64_t dense_occupancy_brick_indices_computation_timer_result_start = 0;
		uint64_t dense_occupancy_brick_indices_computation_timer_result_end = 0;

		return 1e-6 * (dense_occupancy_brick_indices_computation_timer_result_end - dense_occupancy_brick_indices_computation_timer_result_start);
	}

	float OpenGLRenderer::retrieve_isosurface_extraction_computation_time_ms_() {
		uint64_t isosurface_extraction_computation_timer_result_start = 0;
		uint64_t isosurface_extraction_computation_timer_result_end = 0;

		return 1e-6 * (isosurface_extraction_computation_timer_result_end - isosurface_extraction_computation_timer_result_start);
	}

	float OpenGLRenderer::retrieve_meshlet_descriptor_sorting_time_ms_() {
		return 0.0f;
	}


	std::pair<uint32_t, uint32_t> OpenGLRenderer::extract_trimesh_parallel_marching_blocks_(std::vector<uint32_t> const& num_occupied_blocks_host) {
		return { 0, 0 };
	}

	void OpenGLRenderer::compute_min_max_block_texture_() {
		auto const& descriptor_ = *main_volume_descriptor_;

		assert(nullptr != framework_shader_programs_.occupancy.min_max_volume_computation_program_ptr);
		assert(nullptr != main_volume_descriptor_);

		framework_shader_programs_.occupancy.min_max_volume_computation_program_ptr->use();
		framework_shader_programs_.occupancy.min_max_volume_computation_program_ptr->dispatch({ std::ceil(descriptor_.resolution[0] / general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[0]),
			std::ceil(descriptor_.resolution[1] / general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[1]),
			std::ceil(descriptor_.resolution[2] / general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[2]) });

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	void OpenGLRenderer::clear_atomic_counter_buffer_(bool is_in_compute_shader_extraction_mode) const {
		uint32_t const zero = 0u;
		uint32_t const one = 1u;
		glClearNamedBufferSubData(collapsed_atomic_counter_buffers, GL_R32UI, 0, 1024 * sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

		if (is_in_compute_shader_extraction_mode || (!use_NV_mesh_shader_extension_)) {
			glClearNamedBufferSubData(collapsed_atomic_counter_buffers, GL_R32UI, 1 * sizeof(uint32_t), 2 * sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &one);
		}
		else {
			glClearNamedBufferSubData(collapsed_atomic_counter_buffers, GL_R32UI, 1 * sizeof(uint32_t), 1 * sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
		}
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
	
	}

	void OpenGLRenderer::compute_dense_occupancy_brick_indices_() const {

		
		uint32_t num_occupied_blocks = 0;
		assert(nullptr != main_volume_descriptor_);
		auto const& descriptor_ = *main_volume_descriptor_;

		framework_shader_programs_.occupancy.dense_occupancy_computation_program_ptr->use();
		framework_shader_programs_.occupancy.dense_occupancy_computation_program_ptr->dispatch(
			std::ceil((std::ceil(descriptor_.resolution[0] / (static_cast<float>(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[0]) * general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[0])) )),
			std::ceil((std::ceil(descriptor_.resolution[1] / (static_cast<float>(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[1]) * general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[1])) )),
			std::ceil((std::ceil(descriptor_.resolution[2] / (static_cast<float>(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[2]) * general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[2])) ))
		);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
	}

	void OpenGLRenderer::destroy_timer_query_objects_() {
	}

	void OpenGLRenderer::initialize_API_specific_graphics_objects_() {
		BaseRenderer::initialize_API_specific_graphics_objects_();

		allocate_shader_storage_buffer_objects_();
		allocate_storage_buffers_();
		allocate_texture_buffer_objects_();
	}

}