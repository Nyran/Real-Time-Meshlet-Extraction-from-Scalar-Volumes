#ifndef FRAMEWORK_OPENGL_GPU_TIMER_HPP
#define FRAMEWORK_OPENGL_GPU_TIMER_HPP

#include "../base_gpu_timer.hpp"
#include <array>

namespace framework {
	class FRAMEWORK_DLL_EXPORT OpenGLGPUTimer : public BaseGPUTimer {
	public:
		OpenGLGPUTimer(std::string const& timer_name);

		void lazily_create_query_object();

		void start() override;
		void end() override;
		void wait_for_query_result() const override;
		void evaluate_elapsed_time() override;

		void end_wait_and_evaluate() override;



	private:
		uint32_t gl_timer_query_handle_ = 0;
	};
}

#endif //FRAMEWORK_OPENGL_GPU_TIMER_HPP