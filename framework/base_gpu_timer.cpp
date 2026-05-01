#include "base_gpu_timer.hpp"

namespace framework {
	BaseGPUTimer::BaseGPUTimer(std::string const& timer_name) : name_{ timer_name } {

	}

	void BaseGPUTimer::set_measured_elapsed_nanoseconds_(uint64_t measured_elapsed_nanoseconds) {
		elapsed_nanoseconds_ = measured_elapsed_nanoseconds;
	}

	double BaseGPUTimer::get_elapsed_seconds() const {
		return elapsed_nanoseconds_ * 1e-9;
	}

	double BaseGPUTimer::get_elapsed_milliseconds() const {
		return elapsed_nanoseconds_ * 1e-6;
	}

	double BaseGPUTimer::get_elapsed_microseconds() const {
		return elapsed_nanoseconds_ * 1e-3;
	}
}