#include "opengl_volume.hpp"

#include <GL/gl3w.h>


namespace framework {
	OpenGLVolume::OpenGLVolume(std::shared_ptr<framework::OpenGLGraphicsContext> const& opengl_graphics_context, std::string const& resource_path) : BaseVolume(),
		gl_context_{ opengl_graphics_context }
	{
		load_content_from_file_(resource_path);
	}
}
