#ifndef FRAMEWORK_OPENGL_VOLUME_H
#define FRAMEWORK_OPENGL_VOLUME_H

#include "../base_volume.hpp"

#include "opengl_context.hpp"

//reference: meshoptimization / meshlet conversion framework by Arseny Kapoulkine https://github.com/zeux/meshoptimizer
#include <meshoptimizer.h>

#include <array>
#include <vector>

namespace framework {

	class FRAMEWORK_DLL_EXPORT OpenGLVolume : public BaseVolume {
	protected:
		std::shared_ptr<framework::OpenGLGraphicsContext> gl_context_ = nullptr;
		
		uint32_t volume_frame_count_ = 0;
	public:
		OpenGLVolume(std::shared_ptr<framework::OpenGLGraphicsContext> const& opengl_graphics_context, std::string const& resource_path);
		~OpenGLVolume() = default;

	private:
		void upload_to_gpu_() override;
	
	protected:
		// gl variables
		std::vector<uint32_t> gl_volume_textures_;

		uint32_t gl_gpu_writable_min_max_block_texture_ = -1;

		int32_t  gl_internal_volume_format_ = -1;
		int32_t gl_voxel_pixel_format_ = -1;
		int32_t gl_voxel_data_type_ = -1;
	};
}
#endif // FRAMEWORK_OPENGL_VOLUME_H
