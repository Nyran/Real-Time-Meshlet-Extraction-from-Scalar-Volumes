#ifndef FRAMEWORK_OPENGL_GRAPHICS_CONTEXT_H
#define FRAMEWORK_OPENGL_GRAPHICS_CONTEXT_H


#include "../base_graphics_context.hpp"

#include "opengl_extensions_struct.hpp"


namespace framework {

	struct FRAMEWORK_DLL_EXPORT OpenGLGraphicsContext : public BaseGraphicsContext {
	public:
		OpenGLGraphicsContext(glm::ivec2 const& requested_window_resolution);

		OpenGLAPIExtensions loaded_api_extensions;
		OpenGLAPIQueriedParameters queried_api_parameters;

		void swap_buffer_and_poll_events() const;

		void register_gui() const;
		void start_gui_frame() const;
		void end_gui_frame() const;
	protected:
		void create_window_(glm::ivec2 const& requested_window_resolution) override;
		void initialize_() override;
		bool is_in_OpenGL_mode_() override;
	private:
		virtual void register_GL_extensions_();
		virtual void query_GL_parameters_();
	};
}

#endif //FRAMEWORK_OPENGL_GRAPHICS_CONTEXT_H