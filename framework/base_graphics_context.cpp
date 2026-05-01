#include "base_graphics_context.hpp"
#include "imgui/backends/imgui_impl_glfw.h"

#include <iostream>

namespace framework {
	void BaseGraphicsContext::assign_window_(GLFWwindow* window_to_assign) {
		window_ = window_to_assign;
	}

	void BaseGraphicsContext::set_subgroup_size(int32_t subgroup_size) {
		subgroup_size_ = subgroup_size;
	}

	int32_t BaseGraphicsContext::get_subgroup_size() const {
		return subgroup_size_;
	}

	GLFWwindow* BaseGraphicsContext::get_window_handle() const {
		return window_;
	}

	void BaseGraphicsContext::register_key_callback_func(void (*key_callback_func)(GLFWwindow*, int, int, int, int)) {
		if (!window_) {
			std::cout << "No window is available in the graphics context, exiting." << '\n';
			return;
		}

		glfwSetKeyCallback(window_, key_callback_func);
	}

	void BaseGraphicsContext::register_scroll_callback_func(void (*scroll_callback_func)(GLFWwindow* window, double x_offset, double y_offset)) {
		if (!window_) {
			std::cout << "No window is available in the graphics context, exiting." << '\n';
			return;
		}

		glfwSetScrollCallback(window_, scroll_callback_func);
	}

	void BaseGraphicsContext::register_mouse_moved_callback_func(void (*mouse_moved_callback_func)(GLFWwindow*, double, double)) {
		if (!window_) {
			std::cout << "No window is available in the graphics context, exiting." << '\n';
			return;
		}
		


		glfwSetCursorPosCallback(window_, mouse_moved_callback_func);
	}

	void BaseGraphicsContext::register_mouse_clicked_callback_func(void (*mouse_clicked_callback_func)(GLFWwindow*, int, int, int)) {
		if (!window_) {
			std::cout << "No window is available in the graphics context, exiting." << '\n';
			return;
		}

	
		glfwSetMouseButtonCallback(window_, mouse_clicked_callback_func);
	}

}