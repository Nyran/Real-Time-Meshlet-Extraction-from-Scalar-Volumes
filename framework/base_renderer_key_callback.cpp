// framework internal code
#include "base_renderer.hpp"


#include <imgui.h>
// C++ standard libraries
#include <iostream>

namespace framework {

	extern bool needs_stat_refresh;
	void BaseRenderer::key_callback_(GLFWwindow* window, int key, int scancode, int action, int mod) {
		{
			// toggle drawing of gui in general
			if (key == GLFW_KEY_D && action == GLFW_PRESS) {
				draw_gui_ = !draw_gui_;
			}

			// toggle automatic rotation
			if ((GLFW_KEY_R == key) && action == GLFW_PRESS) {

				toggle_auto_rotation();
			}


			if (key == GLFW_KEY_I && action && GLFW_PRESS) {

				toggle_render_isosurface();
			}

			if (key == GLFW_KEY_A && action && GLFW_PRESS) {

				toggle_render_auxiliaries();
			}

			if (GLFW_KEY_C == key && (GLFW_PRESS == action || GLFW_REPEAT == action)) {
				camera_conf.show_window = !camera_conf.show_window;
			}

			if (GLFW_KEY_M == key && (GLFW_PRESS == action || GLFW_REPEAT == action)) {
				model_conf.show_window = !model_conf.show_window;
			}

			// close app on Escape-Keypress
			if ( ((GLFW_KEY_ESCAPE == key) || (GLFW_KEY_Q == key)) && action == GLFW_PRESS) {
				std::cout << "Pressed Escape, exiting app." << '\n';
				glfwSetWindowShouldClose(base_context_->window_, GLFW_TRUE);
			}
		}
	}


	void BaseRenderer::mouse_moved_callback_(GLFWwindow* window, double mouse_pos_x, double mouse_pos_y) {
		if (!NO_GUI) {
			if (ImGui::GetIO().WantCaptureMouse) {
				return;
			}
		}
		std::shared_ptr<framework::BaseCamera> current_pov_camera = get_current_point_of_view_camera();

		current_pov_camera->forward_mouse_motion_state(mouse_pos_x, mouse_pos_y);
	}

	void BaseRenderer::mouse_click_callback_(GLFWwindow* window, int button, int action, int mods) {
		if (!NO_GUI) {
			if (ImGui::GetIO().WantCaptureMouse) {
				return;
			}
		}



		std::shared_ptr<framework::BaseCamera> current_pov_camera = get_current_point_of_view_camera();

		double xpos = 0.0;
		double ypos = 0.0;
		glfwGetCursorPos(window, &xpos, &ypos);
		int mouse_h = xpos;
		int mouse_v = ypos;

		current_pov_camera->forward_mouse_button_state(button, action, mouse_h, mouse_v);
	}

}