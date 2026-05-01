#include "base_camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GLFW/glfw3.h>

#include <cstring>

namespace framework {

	camera_descriptor_t::camera_descriptor_t(
		float p_field_of_view,
		glm::ivec2 const& p_sensor_resolution,
		glm::vec2 const& p_clip_near_far,
		glm::vec3 const& p_camera_position,
		glm::vec3 const& p_camera_up_vector,
		bool p_is_in_OpenGL_mode) :
		field_of_view{ p_field_of_view }, sensor_resolution{ p_sensor_resolution }, clip_near_far{ p_clip_near_far },
		camera_position{ p_camera_position }, camera_up_vector{ p_camera_up_vector },
		is_in_OpenGL_mode{ p_is_in_OpenGL_mode }
	{
		recompute_camera_matrices_();
	}

	void camera_descriptor_t::recompute_camera_matrices_() {

		auto normalize_plane = [](glm::vec4 const& plane_to_normalize) -> glm::vec4 {
			return plane_to_normalize / glm::length(glm::vec3(plane_to_normalize[0], plane_to_normalize[1], plane_to_normalize[2]));
			};

		extrinsics_.camera_mat = glm::inverse(glm::lookAt(camera_position, glm::vec3(0.0f, 0.0f, 0.0f), camera_up_vector));
		extrinsics_.view_mat = glm::inverse(extrinsics_.camera_mat);
		extrinsics_.cam_pos_world_space = extrinsics_.camera_mat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		if (is_in_OpenGL_mode) {
			intrinsics_.projection_mat = glm::perspectiveRH_NO(glm::radians(field_of_view),
				static_cast<float>(sensor_resolution[0]) / static_cast<float>(sensor_resolution[1]),
				clip_near_far[0], clip_near_far[1]);
		}
		else {
			intrinsics_.projection_mat = glm::perspectiveRH_ZO(glm::radians(field_of_view),
				static_cast<float>(sensor_resolution[0]) / static_cast<float>(sensor_resolution[1]),
				clip_near_far[0], clip_near_far[1]);

			intrinsics_.projection_mat[1][1] *= -1;
		}

		intrinsics_.inverse_projection_mat = glm::inverse(intrinsics_.projection_mat);

		//frustum plane computation as in: https://github.com/Nyran/niagara/blob/master/src/niagara.cpp by Arseny Kapoulkine
		glm::mat4 transposed_projection_mat = glm::transpose(intrinsics_.projection_mat);
		glm::vec4 frustumX = normalize_plane(transposed_projection_mat[3] + transposed_projection_mat[0]); // x + w < 0
		glm::vec4 frustumY = normalize_plane(transposed_projection_mat[3] + transposed_projection_mat[1]); // y + w < 0
		intrinsics_.frustum_planes = { frustumX.x, frustumX.z, frustumY.y, frustumY.z };
		intrinsics_.near_far_padding_x2 = { clip_near_far[0], clip_near_far[1], 0.0f, 0.0f };
	}


	BaseCamera::BaseCamera(camera_descriptor_t const& cam_descriptor) : public_descriptor{ cam_descriptor }, internal_descriptor_{ cam_descriptor } {
		navigator_.resize_active_screen_region(cam_descriptor.sensor_resolution[0], cam_descriptor.sensor_resolution[1]);
		update_view_buffer_cpu_data_();
	}

	void BaseCamera::apply_navigation_logic_() {
		user_mode_rotation_matrix_ = navigator_.get_rotation_matrix();
	}


	void BaseCamera::apply_navigation_input_since_last_frame() {
		apply_navigation_logic_();
		navigator_.reset_button_distances();
	}


	void BaseCamera::forward_mouse_button_state(int button, int state, int mouse_pos_horizontal, int mouse_pos_vertical) {
		navigator_.inject_mouse_button_state(button, state, mouse_pos_horizontal, mouse_pos_vertical);
	}

	void BaseCamera::forward_mouse_motion_state(int mouse_pos_horizontal, int mouse_pos_vertical) {
		navigator_.inject_mouse_motion_state(mouse_pos_horizontal, mouse_pos_vertical);
	}

	void BaseCamera::sync_public_with_internal_descriptor_() {
		internal_descriptor_ = public_descriptor;
	}

	void BaseCamera::update_view_buffer_cpu_data_() {
		sync_public_with_internal_descriptor_();
		internal_descriptor_.recompute_camera_matrices_();
		public_descriptor = internal_descriptor_;
	}


	glm::mat4 BaseCamera::get_rotation_matrix() const {
		return user_mode_rotation_matrix_;
	}

	CameraNavigator::CameraNavigator() {
		m_arcball_ptr = std::make_unique<::gl::ArcBall>();
	}

	void CameraNavigator::inject_mouse_button_state(int mouse_button, int button_state, int mouse_pos_horizontal, int mouse_pos_vertical) {
		switch (mouse_button) {
		case GLFW_MOUSE_BUTTON_LEFT: {
			m_arcball_ptr->set_cur(mouse_pos_horizontal, mouse_pos_vertical);
			switch (button_state) {
			case GLFW_PRESS: {
				m_arcball_ptr->begin_drag();
				break;
			}
			case GLFW_RELEASE: {
				m_arcball_ptr->end_drag();
				break;
			}
			default: {
				break;
			}
			}
			break;
		}
		case (GLFW_MOUSE_BUTTON_RIGHT): {
			switch (button_state) {
			case GLFW_PRESS: {
				m_current_button = GLFW_MOUSE_BUTTON_RIGHT;
				m_starting_pos = { mouse_pos_horizontal, mouse_pos_vertical };
				break;
			}
			case GLFW_RELEASE: {
				m_current_button = -1;
				m_starting_pos = { -1.0f, -1.0f };
				break;
			}
			default: {
				break;
			}
			}

			break;
		}
		case (GLFW_MOUSE_BUTTON_MIDDLE): {
			switch (button_state) {
			case GLFW_PRESS: {
				m_current_button = GLFW_MOUSE_BUTTON_LEFT;
				m_starting_pos = { mouse_pos_horizontal, mouse_pos_vertical };
				break;
			}
			case GLFW_RELEASE: {
				m_current_button = -1;
				m_starting_pos = { -1.0f, -1.0f };
				break;
			}
			default: {
				break;
			}
			}
			break;
		}
		default: {
			break;
		}
		}
	}

	void CameraNavigator::inject_mouse_motion_state(int mouse_pos_horizontal, int mouse_pos_vertical) {
		m_arcball_ptr->set_cur(mouse_pos_horizontal, mouse_pos_vertical);
		if (m_current_button >= 0) {
			m_button_distances[m_current_button] = glm::ivec2{ mouse_pos_horizontal, mouse_pos_vertical } - m_starting_pos;
			m_starting_pos = { mouse_pos_horizontal, mouse_pos_vertical };
		}
	}

	void CameraNavigator::resize_active_screen_region(unsigned int screen_width, unsigned screen_height) {
		m_arcball_ptr->set_win_size(screen_width, screen_height);
	}

	glm::dmat4 CameraNavigator::get_rotation_matrix() const {
		glm::mat4 arcball_mat = glm::mat4(1.0);

		memcpy(glm::value_ptr(arcball_mat), m_arcball_ptr->get(), sizeof(glm::mat4));

		return glm::inverse(arcball_mat);
	}

	glm::ivec2 CameraNavigator::get_button_distances(unsigned int button_index) const {
		return m_button_distances[button_index];
	}

	void CameraNavigator::reset_button_distances() {
		m_button_distances = { glm::ivec2{0, 0}, glm::ivec2{0, 0} };
	}

}
