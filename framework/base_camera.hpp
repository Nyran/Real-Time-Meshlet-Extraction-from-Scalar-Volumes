#ifndef FRAMEWORK_CAMERA_HPP
#define FRAMEWORK_CAMERA_HPP

#include "framework_platform.hpp"

#include "CPU_UBO_types.hpp"

#include <arcball_implementation/arcball.hpp>

// external libraries - glm
#include <glm/glm.hpp>
#include <glm/matrix.hpp>

#include <array>
#include <memory>

namespace framework {

	struct extrinsics {
		glm::mat4 camera_mat = glm::mat4(1.0f);
		glm::mat4 view_mat = glm::mat4(1.0f);
		glm::vec4 cam_pos_world_space = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	};

	struct intrinsics {
		glm::mat4 projection_mat = glm::mat4(1.0f);
		glm::mat4 inverse_projection_mat = glm::mat4(1.0f);
		glm::vec4 frustum_planes = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 near_far_padding_x2 = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	};



	class FRAMEWORK_DLL_EXPORT camera_descriptor_t {
	public:
		friend class BaseRenderer;
		camera_descriptor_t(
			float p_field_of_view = 30.0f,
			glm::ivec2 const& p_sensor_resolution = { 100, 100 },
			glm::vec2 const& p_clip_near_far = { 0.01f, 3.0f },
			glm::vec3 const& p_camera_position = glm::vec3{ 0.0f, 0.0f, 2.5f },
			glm::vec3 const& p_camera_up_vector = glm::vec3{ 0.0f, 1.0f, 0.0f },
			bool p_is_in_OpenGL_mode = true);

		float field_of_view;
		glm::ivec2 sensor_resolution;
		glm::vec2 clip_near_far;
		glm::vec3 camera_position;
		glm::vec3 camera_up_vector;
		bool is_in_OpenGL_mode;

		void recompute_camera_matrices_();

	private:
		extrinsics extrinsics_;
		intrinsics intrinsics_;
	};



	class FRAMEWORK_DLL_EXPORT CameraNavigator {
	public:
		CameraNavigator();
		~CameraNavigator() = default;

		void inject_mouse_button_state(int button, int state, int mouse_pos_horizontal, int mouse_pos_vertical);
		void inject_mouse_motion_state(int mouse_pos_horizontal, int mouse_pos_vertical);
		void resize_active_screen_region(unsigned int width, unsigned height);

		glm::dmat4 get_rotation_matrix() const;
		glm::ivec2 get_button_distances(unsigned int button_index) const;
		void reset_button_distances();
	private:
		std::array<glm::dvec2, 2> m_button_distances = { glm::dvec2{0.0f, 0.0f},
														 glm::dvec2{0.0f, 0.0f} };

		glm::ivec2 m_starting_pos = { -1, -1 };
		int m_current_button = -1;

		std::unique_ptr<::gl::ArcBall> m_arcball_ptr = nullptr;
	};


	class FRAMEWORK_DLL_EXPORT BaseCamera {
	public:
		friend class BaseVolume;
		friend class OpenGLVolume;

		friend class BaseRenderer;

		BaseCamera(camera_descriptor_t const& cam_descriptor = {});

		camera_descriptor_t public_descriptor;


		void apply_navigation_input_since_last_frame();

		void forward_mouse_button_state(int button, int state, int mouse_pos_horizontal, int mouse_pos_vertical);
		void forward_mouse_motion_state(int mouse_pos_horizontal, int mouse_pos_vertical);

		glm::mat4 get_rotation_matrix() const;

	protected:
		camera_descriptor_t internal_descriptor_;
		void update_view_buffer_cpu_data_();

	private:
		void apply_navigation_logic_();
		void sync_public_with_internal_descriptor_();
		CameraNavigator navigator_;

		glm::mat4 user_mode_rotation_matrix_ = glm::mat4(1.0f);
	};
}

#endif //FRAMEWORK_CAMERA_HPP
