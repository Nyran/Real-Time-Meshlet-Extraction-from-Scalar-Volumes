#ifndef FRAMEWORK_BASE_VOLUME_TYPES_H
#define FRAMEWORK_BASE_VOLUME_TYPES_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <unordered_map>
#include <vector>


namespace framework {

	enum class VolumeType {
		UNSIGNED_1x8 = 0,
		UNSIGNED_1x16 = 1,
		UNSIGNED_1x32 = 2,

		FLOAT_1x32 = 3,

		NUM_VOLUME_TYPES = 4
	};

	class BytesPerVoxel {
	public:
		static size_t query(VolumeType const& vol_type) {
			return assignment_[vol_type];
		}
	private:

		static std::unordered_map<VolumeType, size_t> assignment_;
	};

	struct VolumeDescriptor {
		glm::uvec3 resolution = { 0, 0, 0 };
		VolumeType type = VolumeType::UNSIGNED_1x8;
		glm::uvec3 num_max_compute_shader_cull_groups = { 0, 0, 0 };
		uint32_t num_ssbo_elements_to_clear = 0;
		std::vector<std::string> volume_paths;
		uint32_t num_volumes_in_sequence = 1;
	};

}

#endif //FRAMEWORK_BASE_VOLUME_TYPES_H
