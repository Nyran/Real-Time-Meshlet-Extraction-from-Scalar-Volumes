#ifndef FRAMEWORK_OPENGL_UBOS_HPP
#define FRAMEWORK_OPENGL_UBOS_HPP

#include "../CPU_UBO_types.hpp" 

#include "meshoptimizer.h"

#include <vector>

namespace framework {
	struct gpu_opengl_culling_reference_camera_buffer {
		cpu_culling_reference_camera_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;
	};

	struct gpu_opengl_point_of_view_camera_buffer {
		cpu_point_of_view_camera_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;
	};

	struct gpu_opengl_model_buffer {
		cpu_model_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;
	};

	struct gpu_opengl_combined_model_culling_reference_camera_buffer {
		cpu_combined_model_culling_reference_camera_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;
	};

	struct gpu_opengl_combined_model_point_of_view_camera_buffer {
		cpu_combined_model_point_of_view_camera_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;
	};


	struct gpu_opengl_draw_parameters_buffer {
		cpu_draw_parameters_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;

	};

	struct gpu_opengl_extraction_parameters_buffer {
		cpu_extraction_parameters_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;
	};

	struct gpu_opengl_fbo_bindless_texture_adresses_buffer {
		cpu_fbo_bindless_texture_adresses_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;
	};

	struct gpu_opengl_meshlet_descriptor_buffer {
		uint32_t gl_buffer = 0;
	};


	struct gpu_opengl_render_frame_descriptor_buffer {
		cpu_render_frame_descriptor_buffer cpu_type_memory;
		uint32_t gl_buffer = 0;
	};


}

#endif //FRAMEWORK_OPENGL_UBOS_HPP