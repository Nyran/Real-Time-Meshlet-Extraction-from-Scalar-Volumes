#ifndef FRAMEWORK_BASE_GPU_TIMER_HPP
#define FRAMEWORK_BASE_GPU_TIMER_HPP

#include "framework_platform.hpp"

#include <string>
#include <cstdint>

namespace framework {
	class FRAMEWORK_DLL_EXPORT BaseGPUTimer {
	public:
		BaseGPUTimer(std::string const& timer_name);

		virtual void start() = 0;
		virtual void end() = 0;
		virtual void wait_for_query_result() const = 0;
		virtual void evaluate_elapsed_time() = 0;

		virtual void end_wait_and_evaluate() = 0;


		//methods to convert last amount of recorded nanoseconds to time units with different granularities
		double get_elapsed_seconds() const;
		double get_elapsed_milliseconds() const;
		double get_elapsed_microseconds() const;

	protected:
		void set_measured_elapsed_nanoseconds_(uint64_t measured_elapsed_nanoseconds);

	private:
		std::string name_ = "TIMER_NAME_NOT_DEFINED";
		uint64_t elapsed_nanoseconds_ = 0u;
	};
}

#endif //FRAMEWORK_BASE_GPU_TIMER_HPP