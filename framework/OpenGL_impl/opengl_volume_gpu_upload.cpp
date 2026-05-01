#include "opengl_volume.hpp"

#include <GL/gl3w.h>

#include <cmath>
#include <iostream>

// uploads the volume data and allocates the GPU-writable per-block min/max texture used by the extraction pipeline
namespace framework {

	void OpenGLVolume::upload_to_gpu_() {

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);

		if (gl_volume_textures_.empty()) {
			gl_volume_textures_.resize(descriptor_.num_volumes_in_sequence, 0);

			glGenTextures(descriptor_.num_volumes_in_sequence, &gl_volume_textures_[0]);
			gl_internal_volume_format_ = -1;
			gl_voxel_pixel_format_ = GL_RED;

			gl_voxel_data_type_ = -1;

			switch (descriptor_.type) {
			case VolumeType::UNSIGNED_1x8: {
				gl_internal_volume_format_ = GL_R8;
				gl_voxel_data_type_ = GL_UNSIGNED_BYTE;
				break;
			}
			case VolumeType::UNSIGNED_1x16: {
				gl_internal_volume_format_ = GL_R16;
				gl_voxel_data_type_ = GL_UNSIGNED_SHORT;
				break;
			}
			case VolumeType::UNSIGNED_1x32: {
				gl_internal_volume_format_ = GL_R32F;
				gl_voxel_data_type_ = GL_FLOAT;
				break;
			}
			default: {
				throw "Did not implement volume type";
			}
			}



			for (uint32_t volume_to_prepare_idx = 0; volume_to_prepare_idx < descriptor_.num_volumes_in_sequence; ++volume_to_prepare_idx) {
				glBindTexture(GL_TEXTURE_3D, gl_volume_textures_[volume_to_prepare_idx]);

				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);

				glTexStorage3D(GL_TEXTURE_3D, 1, gl_internal_volume_format_, descriptor_.resolution[0], descriptor_.resolution[1], descriptor_.resolution[2]);
				glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, descriptor_.resolution[0], descriptor_.resolution[1], descriptor_.resolution[2], gl_voxel_pixel_format_, gl_voxel_data_type_, (char*)cpu_data_volumes_[volume_to_prepare_idx].data());
			}


			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_3D, gl_volume_textures_[0]);

			glActiveTexture(GL_TEXTURE0 + 2);
			glCreateTextures(GL_TEXTURE_3D, 1, &gl_gpu_writable_min_max_block_texture_);

			glTextureStorage3D(gl_gpu_writable_min_max_block_texture_, 1, GL_RG32F,
				std::ceil(descriptor_.resolution[0] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[0])),
				std::ceil(descriptor_.resolution[1] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[1])),
				std::ceil(descriptor_.resolution[2] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[2])));

			glBindTexture(GL_TEXTURE_3D, gl_gpu_writable_min_max_block_texture_);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);

			glBindImageTexture(2, gl_gpu_writable_min_max_block_texture_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
		}
	}
}
