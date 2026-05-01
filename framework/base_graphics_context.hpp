#ifndef FRAMEWORK_BASE_GRAPHICS_CONTEXT_H
#define FRAMEWORK_BASE_GRAPHICS_CONTEXT_H

#include "config/framework_cmake_config.hpp"
#include "framework_platform.hpp"

// external libraries: GL and glmath related

#include <glm/vec2.hpp>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

namespace framework {

	struct FRAMEWORK_DLL_EXPORT BaseGraphicsContext {
		friend class BaseRenderer;

	public:
		int32_t get_subgroup_size() const;
		void set_subgroup_size(int32_t subgroup_size);
	protected:
		virtual void assign_window_(GLFWwindow* window_to_assign);
		GLFWwindow* get_window_handle() const;


	private:
		int32_t subgroup_size_ = 32; // subgroup size, defaults to 32 for the OpenGL version (since at the time of writing there is only an NV extension available)

		virtual void create_window_(glm::ivec2 const& requested_window_resolution) = 0;
		virtual void initialize_() = 0;

		virtual bool is_in_OpenGL_mode_() = 0;

		void register_key_callback_func(void (*key_callback_func)(GLFWwindow*, int, int, int, int));
		void register_scroll_callback_func(void (*key_callback_func)(GLFWwindow*, double, double) );

		void register_mouse_moved_callback_func(void (*key_callback_func)(GLFWwindow*, double, double));
		void register_mouse_clicked_callback_func(void (*key_callback_func)(GLFWwindow*, int , int , int));
		GLFWwindow* window_ = nullptr;
	};

}

#endif //FRAMEWORK_BASE_GRAPHICS_CONTEXT_H