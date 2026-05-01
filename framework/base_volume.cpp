#include "base_volume.hpp"
#include "framework_utils.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <iostream>
#include <map>



namespace framework {
	general_rendering_settings* general_rendering_settings::instance = nullptr;


	general_rendering_settings* general_rendering_settings::get_instance() {
		if (nullptr == instance) {
			instance = new general_rendering_settings();
		}

		return instance;
	}

	std::unordered_map<VolumeType, size_t> BytesPerVoxel::assignment_ = { {VolumeType::UNSIGNED_1x8, sizeof(uint8_t)},
																			{VolumeType::UNSIGNED_1x16, sizeof(uint16_t)},
																			{VolumeType::UNSIGNED_1x32, sizeof(uint32_t)},
																			{VolumeType::FLOAT_1x32, sizeof(float)} };
	
	BaseVolume::BaseVolume() : descriptor_{}, is_cpu_memory_allocated_{ false }, total_num_voxels_{ 0 } {

	}


	void BaseVolume::allocate_volume(VolumeDescriptor const& descriptor) {
		total_num_voxels_ = 1;

		for (uint32_t dim_idx = 0; dim_idx < 3; ++dim_idx) {
			total_num_voxels_ *= size_t{ descriptor_.resolution[dim_idx] };
		}

		size_t num_bytes_per_voxel = BytesPerVoxel::query(descriptor_.type);
		size_t total_num_bytes_in_unpadded_volume = num_bytes_per_voxel * total_num_voxels_;

		// allocate enough space for a sequence of volume data sets
		cpu_data_volumes_
			= std::vector<std::vector<uint8_t>>(descriptor_.num_volumes_in_sequence,
				std::vector<uint8_t>(total_num_bytes_in_unpadded_volume, 0));
	}

	void  BaseVolume::compute_voxel_size_() {
		for (uint32_t dim_idx = 0; dim_idx < 3; ++dim_idx) {
			voxel_size_[dim_idx] = { 1.0f / descriptor_.resolution[dim_idx] };
		}
	}

	void BaseVolume::load_content_from_file_(std::string const& in_volume_path) {
		std::map<std::string, uint64_t> parsed_raw_volume_tokens;
		util::split_volume_filename(in_volume_path, parsed_raw_volume_tokens);


		glm::uvec3 const parsed_resolution = { parsed_raw_volume_tokens["w"], parsed_raw_volume_tokens["h"], parsed_raw_volume_tokens["d"] };


		std::cout << "Parsed volume with dimensions: " << parsed_resolution[0] << ", " << parsed_resolution[1] << ", " << parsed_resolution[2] << '\n';

		int64_t const num_bits_per_voxel = parsed_raw_volume_tokens["b"];

		switch (num_bits_per_voxel) {
			case 8: {
				descriptor_ = VolumeDescriptor{ parsed_resolution , VolumeType::UNSIGNED_1x8 };
				break;
			}
			case 16: {
				descriptor_ = VolumeDescriptor{ parsed_resolution , VolumeType::UNSIGNED_1x16 };
				break;
			}
			case 32: {
				descriptor_ = VolumeDescriptor{ parsed_resolution , VolumeType::UNSIGNED_1x32 };
				break;
			}
			default: {
				throw("Unknown volume format.");
				break;
			}
					   
		}


		std::vector<std::string> volume_files_to_parse;

		// is the volume actually a time series of volumes?
		if (parsed_raw_volume_tokens["s"] > 0) { //yes -> parse the time series
			descriptor_.num_volumes_in_sequence = uint32_t(parsed_raw_volume_tokens["s"]);

			std::string const volume_descriptor_file_name = in_volume_path;

			std::ifstream volume_resource_file(volume_descriptor_file_name, std::ios::in);
			if (!volume_resource_file.is_open()) {
				std::cout << "Could not open volume resource file." << '\n';
				std::cout << "Exiting." << '\n';
				exit(-1);
			}

			for (uint32_t volume_line_to_parse_idx = 0;
				volume_line_to_parse_idx < descriptor_.num_volumes_in_sequence;
				++volume_line_to_parse_idx) {
				std::string parsed_line;
				std::getline(volume_resource_file, parsed_line);
				volume_files_to_parse.push_back(parsed_line);
			}

			volume_resource_file.close();

		}
		else {
			volume_files_to_parse.push_back(in_volume_path);
		}

			
		compute_voxel_size_();

		allocate_volume(descriptor_);

		uint64_t const num_bytes_to_read_per_volume = total_num_voxels_ * num_bits_per_voxel / 8;
		uint32_t volume_to_parse_idx = 0;
		for (auto const& volume_to_load_file_path : volume_files_to_parse) {
			//load volume plain for now
			std::ifstream in_raw_volume(volume_to_load_file_path, std::ios::in | std::ios::binary);
			if (!in_raw_volume.is_open()) {
				std::cout << "Could not open raw volume file: " << volume_to_load_file_path << '\n';
				exit(-1);
			}


			in_raw_volume.read((char*)(cpu_data_volumes_[volume_to_parse_idx].data()), num_bytes_to_read_per_volume);
			in_raw_volume.close();

			++volume_to_parse_idx;

			descriptor_.volume_paths.push_back(volume_to_load_file_path);
		}


		glm::uvec3 const num_blocks_per_axis =
		{ std::ceil(descriptor_.resolution[0] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[0])),
		  std::ceil(descriptor_.resolution[1] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[1])),
		  std::ceil(descriptor_.resolution[2] / float(general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[2])) };

		uint32_t const MAX_NUMBER_OF_TOPOLOGY_CONFIGURATIONS_PER_BLOCK =
			general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[0] *
			general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[1] *
			general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE[2];

		uint64_t const total_number_of_blocks_in_slab = static_cast<uint64_t>(num_blocks_per_axis[0]) * static_cast<uint64_t>(num_blocks_per_axis[1]);

		uint64_t const total_number_of_blocks_in_volume = total_number_of_blocks_in_slab * static_cast<uint64_t>(num_blocks_per_axis[2]);

		upload_to_gpu_();
	}


	glm::uvec3 BaseVolume::get_resolution() const {
		return descriptor_.resolution;
	}

	VolumeDescriptor BaseVolume::get_descriptor() const {
		return descriptor_;
	}
}
