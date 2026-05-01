#include "opengl_gpu_timer.hpp"

// external libraries: GL and GL-Math related
#include <GL/gl3w.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

namespace framework {
	OpenGLGPUTimer::OpenGLGPUTimer(std::string const& timer_name) : BaseGPUTimer { timer_name }{

	}

	void OpenGLGPUTimer::lazily_create_query_object() {
		if (0 == gl_timer_query_handle_) {
			glCreateQueries(GL_TIME_ELAPSED, 1, &gl_timer_query_handle_);
		}
	}

	void OpenGLGPUTimer::start() {
		lazily_create_query_object();

		glBeginQuery(GL_TIME_ELAPSED, gl_timer_query_handle_);
	}

	void OpenGLGPUTimer::end() {
		glEndQuery(GL_TIME_ELAPSED);
	}

	void OpenGLGPUTimer::wait_for_query_result() const {

		int32_t elapsed_timer_query_is_available = 0;

		while (!elapsed_timer_query_is_available) {
			glGetQueryObjectiv(gl_timer_query_handle_,
				GL_QUERY_RESULT_AVAILABLE,
				&elapsed_timer_query_is_available);
		}

	}

	void OpenGLGPUTimer::evaluate_elapsed_time() {
		lazily_create_query_object();
		uint64_t elapsed_time_query_result = 0;
		glGetQueryObjectui64v(gl_timer_query_handle_, GL_QUERY_RESULT, &elapsed_time_query_result);

		set_measured_elapsed_nanoseconds_(elapsed_time_query_result);
	}


	void OpenGLGPUTimer::end_wait_and_evaluate() {
		end();
		wait_for_query_result();
		evaluate_elapsed_time();
	}

}