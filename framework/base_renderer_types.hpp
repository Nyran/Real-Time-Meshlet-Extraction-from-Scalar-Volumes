#ifndef FRAMEWORK_BASE_RENDERER_TYPES_H
#define FRAMEWORK_BASE_RENDERER_TYPES_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <unordered_map>
#include <vector>


namespace framework {

	//each thread analyzes the visibility state of one thread and sets one bit -> 1:1
	int32_t constexpr NUM_MACRO_BLOCKS_PER_THREAD_HANDLED_IN_COMPACTIFICATION_STAGE = 1u;
	//this is slightly arbitrary at the moment but can be calibrated per machine
	int32_t constexpr NUM_THREADS_PER_WORK_GROUP_IN_COMPACTIFICATION_STAGE = 1024u;

	int32_t constexpr WORKGROUP_DIVISOR_FOR_COMPACTIFICATION_STAGE = NUM_MACRO_BLOCKS_PER_THREAD_HANDLED_IN_COMPACTIFICATION_STAGE * NUM_THREADS_PER_WORK_GROUP_IN_COMPACTIFICATION_STAGE;

	// each thread handles 32 compactified bits: -> 32:1
	int32_t constexpr NUM_MACRO_BLOCKS_PER_THREAD_HANDLED_IN_BUILD_OUTPUT_STAGE = 32u;
	int32_t constexpr NUM_THREADS_PER_WORK_GROUP_IN_BUILD_OUTPUT_STAGE = 32u;
	int32_t constexpr WORKGROUP_DIVISOR_FOR_BUILD_OUTPUT_STAGE = NUM_MACRO_BLOCKS_PER_THREAD_HANDLED_IN_BUILD_OUTPUT_STAGE * NUM_THREADS_PER_WORK_GROUP_IN_BUILD_OUTPUT_STAGE;



	struct IndirectDrawStructTaskMeshCommand {
		uint32_t  count;
		uint32_t  instanceCount;
		uint32_t  first;
		uint32_t  baseInstance;
	};

	struct DrawElementsIndirectCommand {
		uint32_t  count = 0;
		uint32_t  instanceCount = 1;
		uint32_t  firstIndex = 0;
		int32_t  baseVertex = 0;
		uint32_t  baseInstance = 0;
	};

	enum class BlockOccupancyMode {
		MIN_MAX_BINARY_OCCUPANCY = 0,
	};

	enum class IndirectRenderingComputeMode {
		INVALID_COMPUTE_MODE = 0,
		COMPUTE_SHADER_BASED_EXTRACTION = 1,
		TASK_MESH_SHADER_BASED_EXTRACTION = 2,
		MESHOPT_OFFLINE_MESHLET_COMPUTATION = 3
	};

	enum class ComputeShaderExtractionMode {
		SINGLE_INDEX_ARRAY = 0,
		BLOCKWISE_32BIT_INDICES = 1,
		BLOCKWISE_16BIT_INDICES = 2
	};

	enum class PersistentExtractionMode {
		VOLUME_SPACE_POSITION_AND_NORMAL = 0,

		MAX_OCCUPIED_PMB_CELLS_32  = 0,
		MAX_OCCUPIED_PMB_CELLS_64  = 1 << 10,
		MAX_OCCUPIED_PMB_CELLS_96  = 1 << 11,
		MAX_OCCUPIED_PMB_CELLS_128 = 1 << 12,
	};

	enum class IndexedMeshVisualizationMode {
		PHONG_SHADED = 0,

		NUM_INDEXED_MESH_SHADER_VIS_PROGRAMS
	};


	enum class MeshletForwardRenderPipelineMode {
		MESH_SHADER_ONLY_NO_CULLING = 0,
		TASK_AND_MESH_SHADER_ONLINE_CULLING = 1,
	};
	

	enum class MeshletVisualizationMode {
		PHONG_SHADED = 0,

		NUM_TASK_MESH_SHADER_VIS_PROGRAMS
	};
	
	enum class shader_variants_t {
		MAX_NUM_VERTICES_128_MAX_NUM_TRIANGLES_254 = 32,
		RENDER_ON_THE_FLY_EXTRACTED_MESHLETS = 256,
		TASK_MESH_SHADER_COMBINATION = 1 << 20,
	};

	enum class parallel_marching_blocks_compute_shader_variants_t {
		PMB_POSITION__AND_NORMAL__AND_LARGE_32_BIT_INDEX_ARRAY = 0,

		PMB_BLOCKWISE_POSITION__AND_NORMAL__AND_32_BIT_INDICES_PER_BLOCK = 8,
		PMB_BLOCKWISE_POSITION__AND_NORMAL__AND_16_BIT_INDICES_PER_BLOCK = 16,
	};

	enum class block_proxy_rasterization_shader_variants_t {
		PASS_THROUGH_PROXY_COLOR = 1
	};

	struct initial_model_transformation_descriptor_t {
		float yaw = 0.0f;
		float pitch = 0.0f;
		float roll = 0.0f;
	};


	enum class timer_mode {
		GLOBAL_FRAME = 0,
		PER_PIPELINE_COMPONENT = 1
	};

	enum class memory_footprint_mode {
		NONE = 0,
		GLOBAL_FRAME = 1,
		PER_MEMORY_COMPONENT = 2
	};


}

#endif //FRAMEWORK_BASE_RENDERER_TYPES_H