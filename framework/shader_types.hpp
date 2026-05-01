#ifndef FRAMEWORK_SHADER_TYPES_HPP
#define FRAMEWORK_SHADER_TYPES_HPP

#include "framework_cmake_config.hpp"

#include <shaderc/shaderc.hpp>

// external: GL definitions
#include <GL/glcorearb.h>

#include <string>
#include <unordered_map>
namespace framework {


enum class shader_types_t {
  //for OpenGL at the point of writing we only have the NV-based mesh shader extension available
  TASK     = GL_TASK_SHADER_NV,
  MESH     = GL_MESH_SHADER_NV,
 
  VERTEX = GL_VERTEX_SHADER,
  TESSELATION_CONTROL = GL_TESS_CONTROL_SHADER,
  TESSELATION_EVALUATION = GL_TESS_EVALUATION_SHADER,
  GEOMETRY = GL_GEOMETRY_SHADER,
  FRAGMENT = GL_FRAGMENT_SHADER,

  COMPUTE  = GL_COMPUTE_SHADER
};

using associated_shader_code_t = std::pair<shader_types_t, std::string>;

inline shaderc_shader_kind map_own_shadertype_to_shaderc_kind(shader_types_t s_type) {
	shaderc_shader_kind kind = shaderc_glsl_infer_from_source;
	switch (s_type) {
	case shader_types_t::TASK: { kind = shaderc_glsl_task_shader; break; }
	case shader_types_t::MESH: { kind = shaderc_glsl_mesh_shader; break; }
	case shader_types_t::VERTEX: { kind = shaderc_glsl_vertex_shader; break; }
	case shader_types_t::TESSELATION_CONTROL: { kind = shaderc_glsl_tess_control_shader; break; }
	case shader_types_t::TESSELATION_EVALUATION: { kind = shaderc_glsl_tess_evaluation_shader; break; }
	case shader_types_t::GEOMETRY: { kind = shaderc_glsl_geometry_shader; break; }
	case shader_types_t::FRAGMENT: { kind = shaderc_glsl_fragment_shader; break; }
	case shader_types_t::COMPUTE: { kind = shaderc_glsl_compute_shader; break; }
	default:
		break;
	}
	return kind;
}

inline std::string lookup_shader_name(shader_types_t s_type) {
	std::string name = "UNKNOWN Shader";
	switch (s_type) {
	case shader_types_t::TASK: {name = "Task Shader"; break;}
	case shader_types_t::MESH: {name = "Mesh Shader"; break;}
	case shader_types_t::VERTEX: {name = "Vertex Shader"; break; }
	case shader_types_t::TESSELATION_CONTROL: {name = "Tess. Con. Shader"; break; }
	case shader_types_t::TESSELATION_EVALUATION: {name = "Tess. Ev. Shader"; break; }
	case shader_types_t::GEOMETRY: {name = "Geometry Shader"; break; }
	case shader_types_t::FRAGMENT: {name = "Fragment Shader"; break; }
	case shader_types_t::COMPUTE: {name = "Compute Shader"; break; }

	default:
		break;
	}

	return name;
}

inline std::string lookup_shader_extension(shader_types_t s_type) {
	std::string extension = ".UNKNOWN";
	switch (s_type) {
	case shader_types_t::TASK: { extension = ".task"; break; }
	case shader_types_t::MESH: { extension = ".mesh"; break; }
	case shader_types_t::VERTEX: { extension = ".vert"; break; }
	case shader_types_t::TESSELATION_CONTROL: { extension = ".tess_cont"; break; }
	case shader_types_t::TESSELATION_EVALUATION: { extension = ".tess_eval"; break; }
	case shader_types_t::GEOMETRY: { extension = ".geom"; break; }
	case shader_types_t::FRAGMENT: { extension = ".frag"; break; }
	case shader_types_t::COMPUTE: { extension = ".comp"; break; }

	default:
		break;
	}

	return extension;
}


} //framework

#endif //FRAMEWORK_SHADER_TYPES_HPP