#ifndef FRAMEWORK_VOLUME_H
#define FRAMEWORK_VOLUME_H

// framework internal
#include "framework_platform.hpp"

#include "base_volume_types.hpp"


#include "shader_program.hpp"

// external libraries - glm
#include <glm/glm.hpp>


// C++ standard libraries
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>



namespace framework {
	
	class FRAMEWORK_DLL_EXPORT general_rendering_settings {
		public:
			static general_rendering_settings* get_instance();

			glm::uvec3 PERSISTENT_EXTRACTION_BLOCK_SIZE = { 4, 4, 4 };
		private:
			static general_rendering_settings* instance;
			general_rendering_settings() = default;
	};

	class FRAMEWORK_DLL_EXPORT BaseVolume {
	public:


		BaseVolume();

		void allocate_volume(VolumeDescriptor const& descriptor);
		void load_content_from_file_(std::string const& in_volume_path);

		glm::uvec3 get_resolution() const;

		VolumeDescriptor get_descriptor() const;


		protected:
		VolumeDescriptor descriptor_;
		std::vector<std::vector<uint8_t>> cpu_data_volumes_;

		bool is_cpu_memory_allocated_;

		uint64_t total_num_voxels_;

		/* pure virtual method, specific API implementation takes care of
		   compute min max hierarchies, and upload volume and hierarchy
		   to 3D textures -- refer to 'OpenGL_impl/opengl_volume_gpu_upload.cpp'
		*/
		virtual void upload_to_gpu_() = 0;

	

		std::chrono::time_point<std::chrono::system_clock> timestamp_last_frame_ = std::chrono::system_clock::now();
		double total_num_seconds_passed = 0.0f;

		bool are_resources_initialized = false;


	protected:
		float voxel_size_[3] = { 1.0f, 1.0f, 1.0f };

	private:
		void compute_voxel_size_();

	
	};
}

#endif //FRAMEWORK_VOLUME_H