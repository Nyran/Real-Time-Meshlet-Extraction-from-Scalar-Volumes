#ifndef FRAMEWORK_CPU_UBO_TYPES_HPP
#define FRAMEWORK_CPU_UBO_TYPES_HPP

#include "framework_platform.hpp"

#include "glm/glm.hpp"

#include <cstring>

namespace framework {

	// UBOs for corresponding GPU types UBO will be bound to binding point 0, if indices have to be used
	struct cpu_culling_reference_camera_buffer {
		glm::mat4 camera_mat = glm::mat4(1.0f);
		glm::mat4 view_mat = glm::mat4(1.0f);
		glm::mat4 projection_mat = glm::mat4(1.0f);
		glm::mat4 inverse_projection_mat = glm::mat4(1.0f);
		glm::mat4 view_projection_mat = glm::mat4(1.0f);
		glm::mat4 inverse_view_projection_mat = glm::mat4(1.0f);
		glm::vec4 pos_world_space = glm::vec4(0.0f, 0.0f, 0.0f, 1.0);

		// computation as in: https://github.com/Nyran/niagara/blob/master/src/niagara.cpp
		// by Arseny Kapoulkine
		glm::vec4 frustum_planes = glm::vec4{};
		glm::vec4 near_far_padding_x2 = glm::vec4{};

		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 4;
	};

	struct cpu_point_of_view_camera_buffer {
		glm::mat4 camera_mat = glm::mat4(1.0f);
		glm::mat4 view_mat = glm::mat4(1.0f);
		glm::mat4 projection_mat = glm::mat4(1.0f);
		glm::mat4 inverse_projection_mat = glm::mat4(1.0f);
		glm::mat4 view_projection_mat = glm::mat4(1.0f);
		glm::mat4 inverse_view_projection_mat = glm::mat4(1.0f);
		glm::vec4 pos_world_space = glm::vec4(0.0f, 0.0f, 0.0f, 1.0);

		// computation as in: https://github.com/Nyran/niagara/blob/master/src/niagara.cpp
		// by Arseny Kapoulkine
		glm::vec4 frustum_planes = glm::vec4{};
		glm::vec4 near_far_padding_x2 = glm::vec4{};

		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 5;
	};


	// UBOs for corresponding GPU types UBO will be bound to binding point 1, if indices have to be used
	struct cpu_model_buffer {
		glm::mat4 model_mat = glm::mat4(1.0f);
		glm::mat4 inv_model_mat = glm::mat4(1.0f);

		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 6;
	};

	struct cpu_combined_model_culling_reference_camera_buffer {
		glm::mat4 model_view_mat = glm::mat4(1.0f);
		glm::mat4 inverse_model_view_mat = glm::mat4(1.0f);
		glm::mat4 normal_mat = glm::mat4(1.0f);
		glm::mat4 model_view_projection_mat = glm::mat4(1.0f);

		glm::vec4 vol_space_cam_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 7;
	};

	struct cpu_combined_model_point_of_view_camera_buffer {
		glm::mat4 model_view_mat = glm::mat4(1.0f);
		glm::mat4 inverse_model_view_mat = glm::mat4(1.0f);
		glm::mat4 normal_mat = glm::mat4(1.0f);
		glm::mat4 model_view_projection_mat = glm::mat4(1.0f);

		glm::vec4 vol_space_cam_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 8;
	};

	// CPU struct defining corresponding GPU structure for UBO -- see specific API implementation
	struct cpu_draw_parameters_buffer {
		glm::vec4 shading_base_color = { 0.5f, 0.5f, 0.5f, 1.0f };

		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 12;
	};


	struct cpu_extraction_parameters_buffer {
		glm::vec4 voxel_size = glm::vec4{};
		glm::ivec4 volume_resolution = glm::ivec4{};
		float iso_value = 0.5f;

		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 13;
	};


	struct cpu_fbo_bindless_texture_adresses_buffer {
		glm::uvec2 resident_handle_color_texture_fbo = { 0, 0 };
		glm::uvec2 resident_handle_depth_texture_fbo = { 0, 0 };

		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 14;
	};

	struct cpu_render_frame_descriptor_buffer {
		uint32_t counter = 0;
		bool is_dirty = true;
		static uint32_t constexpr UBO_BINDING_POINT = 15;
	};



	template <typename CPU_BUFFER_TYPE>
	bool operator!=(CPU_BUFFER_TYPE const& lhs, CPU_BUFFER_TYPE const& rhs) {
		return 0 != std::memcmp((void*)&lhs, (void*)&rhs, sizeof(CPU_BUFFER_TYPE));
	}
}

#endif //FRAMEWORK_CPU_UBO_TYPES_HPP
