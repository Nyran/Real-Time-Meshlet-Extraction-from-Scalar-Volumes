//framework internal
#include "shader_program.hpp"

//externals
//external shader include library by Tahar Meijs https://github.com/tntmeijs/GLSL-Shader-Includes (see externals)
#include <GLSL-Shader-Includes/Shadinclude.hpp>
#include <GL/gl3w.h>

//external: glm
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

//C++ standard libraries
#include <functional>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace framework {

std::vector<std::string> shader_program::static_defines_to_inject_into_each_shader;

std::string shader_program::shader_binary_cache_directory_ = "./GL_shader_binary_cache/";
bool shader_program::enable_binary_cache_ = true;

void shader_program::set_binary_cache_enabled(bool enabled) {
    enable_binary_cache_ = enabled;
}

bool shader_program::_try_load_program_binary(uint32_t program, std::string const& cache_key, std::filesystem::file_time_type source_timestamp) {
    std::string cache_file_path = shader_binary_cache_directory_ + cache_key + ".glbin";

    if (!std::filesystem::exists(cache_file_path)) {
        return false;
    }

    std::ifstream in_file(cache_file_path, std::ios::in | std::ios::binary);
    if (!in_file.is_open()) {
        return false;
    }

    // Read header: format, source timestamp, binary size
    GLenum format = 0;
    int64_t stored_timestamp_count = 0;
    int32_t binary_size = 0;

    in_file.read(reinterpret_cast<char*>(&format), sizeof(format));
    in_file.read(reinterpret_cast<char*>(&stored_timestamp_count), sizeof(stored_timestamp_count));
    in_file.read(reinterpret_cast<char*>(&binary_size), sizeof(binary_size));

    if (!in_file.good() || binary_size <= 0) {
        in_file.close();
        std::filesystem::remove(cache_file_path);
        return false;
    }

    // Check if source is newer than cached binary
    auto stored_timestamp = std::filesystem::file_time_type(std::filesystem::file_time_type::duration(stored_timestamp_count));
    if (source_timestamp > stored_timestamp) {
        in_file.close();
        return false;
    }

    // Read binary data
    std::vector<char> binary_data(binary_size);
    in_file.read(binary_data.data(), binary_size);
    in_file.close();

    if (!in_file.good() && !in_file.eof()) {
        std::filesystem::remove(cache_file_path);
        return false;
    }

    // Load binary into program
    glProgramBinary(program, format, binary_data.data(), binary_size);

    GLint link_status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);

    if (link_status == GL_FALSE) {
        // Binary rejected (driver update, GPU change, etc.)
        std::filesystem::remove(cache_file_path);
        return false;
    }

    return true;
}

void shader_program::_save_program_binary(uint32_t program, std::string const& cache_key, std::filesystem::file_time_type source_timestamp) {
    if (!std::filesystem::exists(shader_binary_cache_directory_)) {
        std::filesystem::create_directories(shader_binary_cache_directory_);
    }

    GLint binary_length = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binary_length);
    if (binary_length <= 0) {
        return;
    }

    std::vector<char> binary_data(binary_length);
    GLenum format = 0;
    glGetProgramBinary(program, binary_length, nullptr, &format, binary_data.data());

    std::string cache_file_path = shader_binary_cache_directory_ + cache_key + ".glbin";
    std::ofstream out_file(cache_file_path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out_file.is_open()) {
        return;
    }

    // Write header: format, source timestamp, binary size
    int64_t timestamp_count = source_timestamp.time_since_epoch().count();
    int32_t binary_size = binary_length;

    out_file.write(reinterpret_cast<char const*>(&format), sizeof(format));
    out_file.write(reinterpret_cast<char const*>(&timestamp_count), sizeof(timestamp_count));
    out_file.write(reinterpret_cast<char const*>(&binary_size), sizeof(binary_size));
    out_file.write(binary_data.data(), binary_size);
    out_file.close();
}

void create_resolved_debug_files(std::string const& unique_shader_identifier, std::string const& shader_filename, std::string const& out_base_folder_name, std::string const& file_extension, std::string const& shader_source_code, bool cache_shader_files) {
    std::regex pattern("^\\.\\/glsl\\/");
    std::filesystem::path resolved_shader_code_path(std::regex_replace(unique_shader_identifier, pattern, out_base_folder_name));
    std::string const resolved_shader_basepath = resolved_shader_code_path.parent_path().string();
    if (!std::filesystem::exists(resolved_shader_basepath)) {
        if (!std::filesystem::create_directories(resolved_shader_basepath)) {
            std::cout << "Could not create resolved-shader cache directories: " << resolved_shader_basepath << '\n';
            exit(-1);
        }
    }
    std::string const resolved_shader_variant_path = resolved_shader_basepath + "/" + shader_filename + file_extension;

    if (cache_shader_files) {
        std::ofstream out_resolved_shader_file(resolved_shader_variant_path, std::ios::out);
        if (!out_resolved_shader_file.is_open()) {
            std::cout << "File is not open." << '\n';
            exit(-1);
        }
        out_resolved_shader_file.write(reinterpret_cast<char const*>(shader_source_code.data()), shader_source_code.size());
        out_resolved_shader_file.close();
    }
}

// shader program convenience functions
shader_program::
shader_program(bool compile_shader_variants_immediately, shader_components const& components, shader_variant_defines_t const& shader_variant_defines, std::set<std::string> const& program_local_defines, bool cache_compiled_shaders, shader_variant_component_blacklist_t const& shader_variant_component_blacklist)
    : pre_compile_shader_programs_{ compile_shader_variants_immediately }, cache_shader_files_{cache_compiled_shaders}
    {

    bool has_mesh_shader = false;

    if (!components.task.path.empty()) {
        shaders_.emplace_back( shader_types_t::TASK, components.task.path);
        shader_base_name_ += "\t" + std::filesystem::path(components.task.path).filename().string() + "\n";
    }
    if (!components.mesh.path.empty()) {
        shaders_.emplace_back(shader_types_t::MESH, components.mesh.path);
        shader_base_name_ += "\t" + std::filesystem::path(components.mesh.path).filename().string() + "\n";
        has_mesh_shader = true;
    }

    if (!components.vertex.path.empty()) {
        shaders_.emplace_back(shader_types_t::VERTEX, components.vertex.path);
        shader_base_name_ += "\t" + std::filesystem::path(components.vertex.path).filename().string() + "\n";
    }
    if (!components.tesselation_control.path.empty()) {
        shaders_.emplace_back(shader_types_t::TESSELATION_CONTROL, components.tesselation_control.path);
        shader_base_name_ += "\t" + std::filesystem::path(components.tesselation_control.path).filename().string() + "\n";
    }
    if (!components.tesselation_control.path.empty()) {
        shaders_.emplace_back(shader_types_t::TESSELATION_EVALUATION, components.tesselation_evaluation.path);
        shader_base_name_ += "\t" + std::filesystem::path(components.tesselation_evaluation.path).filename().string() + "\n";
    }
    if (!components.geometry.path.empty()) {
        shaders_.emplace_back(shader_types_t::GEOMETRY, components.geometry.path);
        shader_base_name_ += "\t" + std::filesystem::path(components.geometry.path).filename().string() + "\n";
    }
    if (!components.fragment.path.empty()) {
        shaders_.emplace_back(shader_types_t::FRAGMENT, components.fragment.path);
        shader_base_name_ += "\t" + std::filesystem::path(components.fragment.path).filename().string() + "\n";
    }

    if (!components.compute.path.empty()) {
        shaders_.emplace_back(shader_types_t::COMPUTE, components.compute.path);
        shader_base_name_ += "\t" + std::filesystem::path(components.compute.path).filename().string() + "\n";
    }

    if (!components.program_name.empty()) {
        user_defined_program_name_ = components.program_name;
    }
    else {
        user_defined_program_name_ = shader_base_name_;
    }

  //create additional "shader variant" with index DEFAULT_SHADER_INDEX (= int min for now), which is the original shader containing only the globally specified defines
  shader_variant_defines_.insert({ DEFAULT_SHADER_INDEX , {} });
  shader_variant_defines_.insert(shader_variant_defines.begin(), shader_variant_defines.end());

  shader_variant_component_blacklist_.insert(shader_variant_component_blacklist.begin(), shader_variant_component_blacklist.end());

  for (auto& shaders : shader_variant_defines_) {
      shader_program_variants_.insert({ shaders.first, 0u });
  }


  program_local_defines_ = program_local_defines;


  if (compile_shader_variants_immediately) {
      if (!_compile_shaders()) {
          std::cout << "Could not compile shaders for program: " << '\n';
          std::cout << "\033[4;95m" << user_defined_program_name_ << "\033[0m" << '\n';

          std::cout << "Program local defines: " << '\n';
          for (auto const& local_define : program_local_defines) {
              std::cout << local_define << '\n';
          }
          std::cout << '\n';
          exit(-1);
      }
  }


}

void shader_program::
use(int32_t shader_variant_index) {
    if (!shader_program_variants_.contains(shader_variant_index)) {
            std::cout << "Shader variant with index " << shader_variant_index << " does not exist for shader program built from:\n\033[1;31m" + (!user_defined_program_name_.empty() ? user_defined_program_name_ : shader_base_name_) + "\033[0m . Cannot use it." << '\n';
            exit(-1);
    }
    
    if (!pre_compile_shader_programs_) {
        if (0 == shader_program_variants_[shader_variant_index]) {
            if (!_compile_shader_variant(shader_variant_index)) {
                std::cout << "Could not create shader program lazily" << '\n';
            }
        }
    }
    glUseProgram(shader_program_variants_[shader_variant_index]);
}


//retrieve all the named and modified shader components for a given variant index
std::vector<associated_shader_code_t> shader_program::
get_associated_shader_code_for_variant(int32_t shader_variant_index) const {
    if (ASCII_individual_shaders_per_shader_variant_.contains(shader_variant_index)) {
        return ASCII_individual_shaders_per_shader_variant_.at(shader_variant_index);
    }

    std::cout << "\x1B[93m";
    std::cout << "Requested shader variant (" << shader_variant_index << ") is not registered, thus source code cannot be queried." << '\n';
    std::cout << "\x1B[93m=========================";
    std::cout << "\033[0m" << '\n';




    return {};
}

void shader_program::
dispatch(glm::uvec3 const& num_workgroups_3d) {
    dispatch(num_workgroups_3d[0], num_workgroups_3d[1], num_workgroups_3d[2]);

}

void shader_program::
dispatch(uint32_t num_workgroups_x, uint32_t num_workgroups_y, uint32_t num_workgroups_z) {
    glDispatchCompute(num_workgroups_x, num_workgroups_y, num_workgroups_z);
}

void shader_program::
dispatch_indirect(uint32_t indirect_buffer_byte_offsetindirect_buffer_byte_offset) {
    glDispatchComputeIndirect(indirect_buffer_byte_offsetindirect_buffer_byte_offset);

}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::mat4 const& uniform, int32_t shader_variant_index) {
  _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
  glUniformMatrix4fv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, GL_FALSE, glm::value_ptr(uniform) );
}


template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::uvec2 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform2uiv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::ivec2 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform2iv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::vec2 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform2fv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::uvec3 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform3uiv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::ivec3 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform3iv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::vec3 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform3fv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::uvec4 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform4uiv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::ivec4 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform4iv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, glm::vec4 const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform4fv(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), 1, glm::value_ptr(uniform));
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, int32_t const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform1i(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), uniform );
}

/*
template <>
void shader_program::
upload_uniform(std::string const& uniform_name, int32_t const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform1i(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), uniform);
}
*/

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, bool const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform1i(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), uniform);
}

template <>
void shader_program::
upload_uniform(std::string const& uniform_name, float const& uniform, int32_t shader_variant_index) {
    _check_whether_shader_variant_exists(shader_variant_index, uniform_name);
    glUniform1f(glGetUniformLocation(shader_program_variants_[shader_variant_index], uniform_name.c_str()), uniform);
}

/*
void shader_program::
hotplug_local_shader_variant_defines_and_recompile(
    int32_t shader_variant_index,
    std::vector<std::string> const& local_shader_variant_program_defines_to_hot_plug) {



    if (!shader_program_variants_.contains(shader_variant_index)) {
        std::cout << "Shader variant with index " << shader_variant_index << " does not exist. Can not hotplug and recompile shader." << std::endl;
        exit(-1);
    }


    auto& last_hot_plugged_shader_program_defines_for_variant = last_used_hot_plugged_local_shader_variant_program_defines_[shader_variant_index];
    if (local_shader_variant_program_defines_to_hot_plug.size() == last_hot_plugged_shader_program_defines_for_variant.size()) {
        bool all_strings_the_same = true;
        

        for (uint32_t strings_to_check_idx = 0; strings_to_check_idx < local_shader_variant_program_defines_to_hot_plug.size(); ++strings_to_check_idx) {
            if (local_shader_variant_program_defines_to_hot_plug[strings_to_check_idx] != last_hot_plugged_shader_program_defines_for_variant[strings_to_check_idx]) {
                all_strings_the_same = false;
            }
        }

        if (true == all_strings_the_same) {
            return; //  nothing to recompile, because hot plugged defines were already applied last frame
        }
    }

    last_hot_plugged_shader_program_defines_for_variant = local_shader_variant_program_defines_to_hot_plug;

    glDeleteProgram(shader_program_variants_[shader_variant_index]);

    last_used_hot_plugged_local_shader_variant_program_defines_.insert_or_assign(shader_variant_index, local_shader_variant_program_defines_to_hot_plug);

    auto shader_to_recompile_pair_iterator = shader_program_variants_.find(shader_variant_index);

    _compile_shader_program_variant(*shader_to_recompile_pair_iterator);

    // recompile shader with all defines and add hot plugged defines prior to recompilation

   //: shader_program_variants_
    
}
*/

bool shader_program::
_read_shader_unmodified(std::string const& shader_file_path, std::string& shader_source_out) {
    std::ifstream in_shader_source_file(shader_file_path, std::ios::in);
    if(!in_shader_source_file.is_open()) {
      std::cout << "Could not find shader file: " << shader_file_path << '\n';
      exit(-1);
      return false;
    }

    std::ostringstream shader_string_stream;
    shader_string_stream << in_shader_source_file.rdbuf();
    shader_source_out = shader_string_stream.str();
 
    in_shader_source_file.close();
    return true;
}


void replace_and_create_directories(const std::string& pathStr, const std::string& from = "glsl", const std::string& to = "glsl_assembled") {
    std::filesystem::path originalPath(pathStr);

    // Convert to absolute for safety (optional)
    originalPath = std::filesystem::absolute(originalPath);

    // Look for the part to replace
    auto it = std::find(originalPath.begin(), originalPath.end(), from);
    if (it == originalPath.end()) {
        std::cerr << "Directory '" << from << "' not found in path: " << pathStr << '\n';
        return;
    }

    // Replace the "glsl" part with "glsl_assembled"
    std::filesystem::path newPath;
    for (auto part = originalPath.begin(); part != originalPath.end(); ++part) {
        if (part == it) {
            newPath /= to;
        }
        else {
            newPath /= *part;
        }
    }

    // Create directories if they don't exist
    try {
        std::filesystem::create_directories(newPath);
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating directories: " << e.what() << '\n';
    }
}


std::string shader_program::_resolve_shader_source_code(std::string const& shader_file_path, int32_t shader_program_variant_index, std::filesystem::file_time_type& freshest_encountered_file_timestamp) {

    //std::filesystem::file_time_type freshest_encountered_file_timestamp = std::filesystem::file_time_type::clock::time_point();

    std::string shader_source_code;

    // use external ShadInclude library to resolve shader includes recursively

    //try opening the file to be able to generate an error message in case the file does not exist
    std::ifstream in_shader_source_file(shader_file_path, std::ios::in);
    if (!in_shader_source_file.is_open()) {
        std::cout << "Could not find shader file: " << shader_file_path << '\n';
        exit(-1);
    }
    in_shader_source_file.close();

    shader_source_code = Shadinclude::load(shader_file_path, freshest_encountered_file_timestamp);

    std::istringstream shader_string_stream(shader_source_code);

    std::string modified_shader_code;

    std::string line_buffer;

    bool is_first_line = true;
    while (std::getline(shader_string_stream, line_buffer)) {
        modified_shader_code += line_buffer + '\n';

        if (is_first_line) {
            is_first_line = false;

            modified_shader_code += "#define OPENGL_PATH\n";
            for (auto const& global_define_line_to_add : static_defines_to_inject_into_each_shader) {
                modified_shader_code += global_define_line_to_add + '\n';
            }

            for (auto const& shader_variant_define_line_to_add : shader_variant_defines_[shader_program_variant_index]) {
                modified_shader_code += shader_variant_define_line_to_add + '\n';
            }

            for (auto const& local_program_define_line_to_add : program_local_defines_) {
                modified_shader_code += local_program_define_line_to_add + '\n';
            }

            if (last_used_hot_plugged_local_shader_variant_program_defines_.contains(shader_program_variant_index)) {
                for (auto const& hot_plugged_local_shader_variant_program_define_line_to_add : last_used_hot_plugged_local_shader_variant_program_defines_[shader_program_variant_index]) {
                    modified_shader_code += hot_plugged_local_shader_variant_program_define_line_to_add + '\n';
                }
            }

        }
    }

    return modified_shader_code;

}

bool shader_program::_compile_shader_program_variant_OpenGL_path(std::pair<int32_t const, uint32_t>& shader_program_variant_and_program_pair) {
    shader_program_variant_and_program_pair.second = glCreateProgram();

    // Enable binary retrieval for caching
    glProgramParameteri(shader_program_variant_and_program_pair.second, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

    // Compute cache key and freshest source timestamp across all shader components
    if (enable_binary_cache_) {
        std::filesystem::file_time_type freshest_source_timestamp = std::filesystem::file_time_type::clock::time_point();

        // Resolve all shader sources to find freshest timestamp AND store resolved sources for reuse on cache miss
        std::vector<std::string> resolved_sources;
        resolved_sources.reserve(shaders_.size());
        for (auto const& shader_file : shaders_) {
            std::filesystem::file_time_type component_timestamp = std::filesystem::file_time_type::clock::time_point();
            resolved_sources.push_back(_resolve_shader_source_code(shader_file.second, shader_program_variant_and_program_pair.first, component_timestamp));
            if (component_timestamp > freshest_source_timestamp) {
                freshest_source_timestamp = component_timestamp;
            }
        }

        // Build cache key: program name + variant index + hash of all defines
        std::string defines_to_hash;
        for (auto const& d : static_defines_to_inject_into_each_shader) { defines_to_hash += d; }
        for (auto const& d : shader_variant_defines_[shader_program_variant_and_program_pair.first]) { defines_to_hash += d; }
        for (auto const& d : program_local_defines_) { defines_to_hash += d; }
        if (last_used_hot_plugged_local_shader_variant_program_defines_.contains(shader_program_variant_and_program_pair.first)) {
            for (auto const& d : last_used_hot_plugged_local_shader_variant_program_defines_[shader_program_variant_and_program_pair.first]) { defines_to_hash += d; }
        }
        size_t defines_hash = std::hash<std::string>{}(defines_to_hash);

        std::string program_label = !user_defined_program_name_.empty() ? user_defined_program_name_ : shader_base_name_;
        // Sanitize program label for file path (replace slashes and other problematic chars)
        for (auto& c : program_label) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                c = '_';
            }
        }
        std::string cache_key = program_label + "_SV" + std::to_string(shader_program_variant_and_program_pair.first) + "_" + std::to_string(defines_hash);

        if (_try_load_program_binary(shader_program_variant_and_program_pair.second, cache_key, freshest_source_timestamp)) {
            return true;
        }

        // Cache miss - compile normally and save after linking
        // Store cache key and timestamp for use after linking
        // (use lambdas captured below)
        std::vector<int32_t> attached_shaders_;
        attached_shaders_.reserve(shaders_.size());

        uint32_t local_shader_index = 0;
        for (size_t shader_idx = 0; shader_idx < shaders_.size(); ++shader_idx) {
            auto const& shader_file = shaders_[shader_idx];
            shaderc_shader_kind shaderc_shader_kind_to_compile = map_own_shadertype_to_shaderc_kind(shader_file.first);

            auto const& blacklist_for_shader_variant_index = shader_variant_component_blacklist_.find(shader_program_variant_and_program_pair.first);
            if (shader_variant_component_blacklist_.end() != blacklist_for_shader_variant_index) {
                if (blacklist_for_shader_variant_index->second.count(shaderc_shader_kind_to_compile) > 0) {
                    continue;
                }
            }

            // Reuse pre-resolved source from timestamp pass (avoids duplicate file I/O)
            std::string shader_source_code = resolved_sources[shader_idx];

            GLuint current_gl_shader = glCreateShader((GLenum)(shader_file.first));
            std::string const unique_shader_identifier = shader_file.second + "_" + std::to_string(shader_program_variant_and_program_pair.first);
            std::string shader_filename = std::filesystem::path(unique_shader_identifier).filename().string();
            create_resolved_debug_files(unique_shader_identifier, shader_filename, "./GL_glsl_input/", ".pre_glsl", shader_source_code, cache_shader_files_);

            auto const shader_source_code_copy = shader_source_code;
            const char* shader_source_as_c_string = shader_source_code.c_str();
            const int shader_source_length = int(shader_source_code.size());
            glShaderSource(current_gl_shader, 1, &shader_source_as_c_string, &shader_source_length);
            glCompileShader(current_gl_shader);

            GLint shader_compilation_success = GL_TRUE;
            glGetShaderiv(current_gl_shader, GL_COMPILE_STATUS, &shader_compilation_success);

            if (GL_FALSE == shader_compilation_success) {
                GLint shader_info_log_length = 0;
                glGetShaderiv(current_gl_shader, GL_INFO_LOG_LENGTH, &shader_info_log_length);
                char* shader_info_log = new char[shader_info_log_length];
                glGetShaderInfoLog(current_gl_shader, shader_info_log_length, NULL, shader_info_log);
                std::cout << '\n';
                std::cout << "\x1B[93m";
                std::cout << "Failed to compile shader: " << shader_file.second << '\n';
                std::cout << "\x1B[93m=========================";
                std::cout << "\033[0m" << '\n';
                std::cout << shader_info_log << '\n';
                delete[] shader_info_log;
                return false;
            }

            glAttachShader(shader_program_variant_and_program_pair.second, current_gl_shader);
            attached_shaders_.push_back(current_gl_shader);
            ++local_shader_index;
        }

        glLinkProgram(shader_program_variant_and_program_pair.second);

        for (auto const& attached_shader : attached_shaders_) {
            glDetachShader(shader_program_variant_and_program_pair.second, attached_shader);
            glDeleteShader(attached_shader);
        }

        GLint program_linking_success = GL_TRUE;
        glGetProgramiv(shader_program_variant_and_program_pair.second, GL_LINK_STATUS, &program_linking_success);

        if (GL_FALSE == program_linking_success) {
            std::cout << "Failed to link program!" << '\n';
            GLint program_info_log_length = 0;
            glGetProgramiv(shader_program_variant_and_program_pair.second, GL_INFO_LOG_LENGTH, &program_info_log_length);
            char* program_info_log = new char[program_info_log_length];
            glGetProgramInfoLog(shader_program_variant_and_program_pair.second, program_info_log_length, NULL, program_info_log);
            std::cout << '\n';
            std::cout << "\x1B[93m";
            std::cout << "Failed to link shader program for shaders: " << '\n';
            for (auto const& shader_file : shaders_) {
                std::cout << "\t" << shader_file.second << '\n';
            }
            std::cout << "\x1B[93m==================================";
            std::cout << "\033[0m" << '\n';
            std::cout << program_info_log << '\n';
            delete[] program_info_log;
            return false;
        }

        // Save binary cache after successful link
        _save_program_binary(shader_program_variant_and_program_pair.second, cache_key, freshest_source_timestamp);
        return true;
    }

    // Non-cached path (original code)
    std::vector<int32_t> attached_shaders_;
    attached_shaders_.reserve(shaders_.size());


    uint32_t local_shader_index = 0;
    for (auto const& shader_file : shaders_) {
        shaderc_shader_kind shaderc_shader_kind_to_compile = map_own_shadertype_to_shaderc_kind(shader_file.first);

        auto const& blacklist_for_shader_variant_index = shader_variant_component_blacklist_.find(shader_program_variant_and_program_pair.first);

        //skip shader components if we find them in the blacklist of the current shader
        if (shader_variant_component_blacklist_.end() != blacklist_for_shader_variant_index) {
            if (blacklist_for_shader_variant_index->second.count(shaderc_shader_kind_to_compile) > 0) {
                continue;
            }
        }


        std::filesystem::file_time_type freshest_encountered_file_timestamp = std::filesystem::file_time_type::clock::time_point();
		std::string shader_source_code = _resolve_shader_source_code(shader_file.second, shader_program_variant_and_program_pair.first, freshest_encountered_file_timestamp);





        //shader_file.first -> our shader
        GLuint current_gl_shader = glCreateShader((GLenum)(shader_file.first));

        std::string const unique_shader_identifier = shader_file.second + "_" + std::to_string(shader_program_variant_and_program_pair.first);
        std::string shader_filename = std::filesystem::path(unique_shader_identifier).filename().string();
        create_resolved_debug_files(unique_shader_identifier,shader_filename, "./GL_glsl_input/", ".pre_glsl", shader_source_code, cache_shader_files_);


        auto const shader_source_code_copy = shader_source_code;
        const char* shader_source_as_c_string = shader_source_code.c_str();
        const int   shader_source_length = int(shader_source_code.size());
        glShaderSource(current_gl_shader, 1, &shader_source_as_c_string, &shader_source_length);

        //std::hash(shader_source_code);

        glCompileShader(current_gl_shader);

        GLint shader_compilation_success = GL_TRUE;
        glGetShaderiv(current_gl_shader, GL_COMPILE_STATUS, &shader_compilation_success);

        if (GL_FALSE == shader_compilation_success) {
            GLint shader_info_log_length = 0;
            glGetShaderiv(current_gl_shader, GL_INFO_LOG_LENGTH, &shader_info_log_length);

            char* shader_info_log = new char[shader_info_log_length];
            glGetShaderInfoLog(current_gl_shader, shader_info_log_length, NULL, shader_info_log);


            std::cout << '\n';
            std::cout << "\x1B[93m";
            std::cout << "Failed to compile shader: " << shader_file.second << '\n';
            std::cout << "\x1B[93m=========================";
            std::cout << "\033[0m" << '\n';
            std::string shader_info_log_as_cpp_string(shader_info_log);


#if 1
            size_t pos = 0;
            std::string token;
            bool is_first = true;

            std::vector < std::string > shader_source_code_line_by_line;
            while ((pos = shader_source_code.find('\n')) != std::string::npos) {
                token = shader_source_code.substr(0, pos);
                if (is_first) {

                }
                shader_source_code_line_by_line.push_back(token);
                if (is_first) {
                    std::cout << "\033[0m";
                    is_first = false;
                }
                shader_source_code.erase(0, pos + 1);
            }


            std::vector < std::string > shader_info_log_line_by_line;
            while ((pos = shader_info_log_as_cpp_string.find('\n')) != std::string::npos) {
                token = shader_info_log_as_cpp_string.substr(0, pos);
                if (is_first) {

                }
                shader_info_log_line_by_line.push_back(token);
                if (is_first) {
                    std::cout << "\033[0m";
                    is_first = false;
                }
                shader_info_log_as_cpp_string.erase(0, pos + 1);
            }

            std::string const digits = "0123456789";
            for (auto const& shader_info_log_line : shader_info_log_line_by_line) {
                std::cout << "\x1B[91m";
                std::cout << shader_info_log_line << "\033[0m" << " -- reference line:" << '\n';


                auto first_digit_pos = shader_info_log_line.find_first_of(digits, 1);
                auto first_non_digit_pos_after_digit = shader_info_log_line.find_first_not_of(digits, first_digit_pos);

                if (first_digit_pos != std::string::npos && first_non_digit_pos_after_digit != std::string::npos) {
                    std::string const index_string = shader_info_log_line.substr(first_digit_pos, first_non_digit_pos_after_digit - first_digit_pos);
                    int const line_index = std::atoi(index_string.c_str());
                    std::cout << shader_source_code_line_by_line[line_index - 1] << '\n';
                }
            }
#else

            std::cout << shader_info_log_as_cpp_string << std::endl;
#endif
            delete[] shader_info_log;



            return false;
        }
        /*
        else {
            
            std::string const assembled_shader_file = "./out_assembled_shader_files/";
            if (!std::filesystem::exists(assembled_shader_file)) {
                std::filesystem::create_directory(assembled_shader_file);
            }

            std::string const assembled_shader_file_path_string = assembled_shader_file + user_defined_program_name_ + lookup_shader_extension(shader_file.first) + "__SV" + std::to_string(shader_program_variant_and_program_pair.first);

            std::ofstream out_assembled_shader_file(assembled_shader_file_path_string, std::ios::out | std::ios::trunc);
            out_assembled_shader_file << shader_source_code_copy;
        
            out_assembled_shader_file.close();
            
        }*/

        glAttachShader(shader_program_variant_and_program_pair.second, current_gl_shader);
        attached_shaders_.push_back(current_gl_shader);
        ++local_shader_index;
    }


    glLinkProgram(shader_program_variant_and_program_pair.second);

    for (auto const& attached_shader : attached_shaders_) {
        glDetachShader(shader_program_variant_and_program_pair.second, attached_shader);
        glDeleteShader(attached_shader);
    }
    //glDetachShader(shader_program_variant_and_program_pair.second);

    GLint program_linking_success = GL_TRUE;
    glGetProgramiv(shader_program_variant_and_program_pair.second, GL_LINK_STATUS, &program_linking_success);

    if (GL_FALSE == program_linking_success) {
        std::cout << "Failed to link program!" << '\n';
        GLint program_info_log_length = 0;
        glGetProgramiv(shader_program_variant_and_program_pair.second, GL_INFO_LOG_LENGTH, &program_info_log_length);

        char* program_info_log = new char[program_info_log_length];
        glGetProgramInfoLog(shader_program_variant_and_program_pair.second, program_info_log_length, NULL, program_info_log);

        std::cout << '\n';
        std::cout << "\x1B[93m";
        std::cout << "Failed to link shader program for shaders: " << '\n';
        for (auto const& shader_file : shaders_) {
            std::cout << "\t" << shader_file.second << '\n';
        }
        std::cout << "\x1B[93m==================================";
        std::cout << "\033[0m" << '\n';
        std::cout << program_info_log << '\n';

        delete[] program_info_log;

        return false;
    }

    int32_t shader_variant_index = shader_program_variant_and_program_pair.first;
    /*ASCII_individual_shaders_per_shader_variant_.insert_or_assign(
           shader_variant_index, individual_shader_container
    );*/


    return true;
}

void shader_program::_check_whether_shader_variant_exists(int32_t shader_variant_index, std::string const& uniform_name) const {
    if (!shader_program_variants_.contains(shader_variant_index)) {
        _print_shader_variant_error_message(shader_variant_index, uniform_name);
        exit(-1);
    }
}

void shader_program::_print_shader_variant_error_message(int32_t shader_variant_index, std::string const& uniform_name) const {
    std::cout << "Shader variant with index " << shader_variant_index << " does not exist for shader program built from:\n\033[1;31m" + (!user_defined_program_name_.empty() ? user_defined_program_name_ : shader_base_name_) + "\033[0m . Cannot Upload uniform (" + uniform_name + ")." << '\n';
}

bool shader_program::_compile_shader_variant(int32_t shader_variant_index) {
    auto shader_program_variant_pair = shader_program_variants_.find(shader_variant_index);

    if (shader_program_variant_pair != shader_program_variants_.end()) {
        bool compilation_successful = _compile_shader_program_variant_OpenGL_path(*shader_program_variant_pair);

        if (!compilation_successful) {
            std::cerr << "Shader variant compilation failed for shader variant with index " << shader_program_variant_pair->first << '\n';
            return false;
        }
    }

    return true;
}

bool shader_program::_compile_shaders() {
    // Batched OpenGL compilation: submit all glCompileShader calls before querying any status,
    // allowing the driver to compile in parallel via ARB_parallel_shader_compile.

    struct pending_variant_t {
        int32_t variant_index;
        uint32_t program;
        std::string cache_key;
        std::filesystem::file_time_type freshest_source_timestamp;
        std::vector<std::string> resolved_sources;
        std::vector<GLuint> shader_handles;
        std::vector<size_t> shader_source_indices; // maps to shaders_ index
    };

    std::vector<pending_variant_t> cache_miss_variants;

    // ---- Pass 1: Resolve sources, attempt cache loads ----
    for (auto& [variant_index, gl_program] : shader_program_variants_) {
        gl_program = glCreateProgram();
        glProgramParameteri(gl_program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

        if (!enable_binary_cache_) {
            // No cache path — must compile
            pending_variant_t pending;
            pending.variant_index = variant_index;
            pending.program = gl_program;
            pending.resolved_sources.reserve(shaders_.size());
            for (auto const& shader_file : shaders_) {
                std::filesystem::file_time_type ts = std::filesystem::file_time_type::clock::time_point();
                pending.resolved_sources.push_back(_resolve_shader_source_code(shader_file.second, variant_index, ts));
            }
            cache_miss_variants.push_back(std::move(pending));
            continue;
        }

        // Resolve all sources and find freshest timestamp
        std::filesystem::file_time_type freshest_source_timestamp = std::filesystem::file_time_type::clock::time_point();
        std::vector<std::string> resolved_sources;
        resolved_sources.reserve(shaders_.size());
        for (auto const& shader_file : shaders_) {
            std::filesystem::file_time_type component_timestamp = std::filesystem::file_time_type::clock::time_point();
            resolved_sources.push_back(_resolve_shader_source_code(shader_file.second, variant_index, component_timestamp));
            if (component_timestamp > freshest_source_timestamp) {
                freshest_source_timestamp = component_timestamp;
            }
        }

        // Build cache key
        std::string defines_to_hash;
        for (auto const& d : static_defines_to_inject_into_each_shader) { defines_to_hash += d; }
        for (auto const& d : shader_variant_defines_[variant_index]) { defines_to_hash += d; }
        for (auto const& d : program_local_defines_) { defines_to_hash += d; }
        if (last_used_hot_plugged_local_shader_variant_program_defines_.contains(variant_index)) {
            for (auto const& d : last_used_hot_plugged_local_shader_variant_program_defines_[variant_index]) { defines_to_hash += d; }
        }
        size_t defines_hash = std::hash<std::string>{}(defines_to_hash);

        std::string program_label = !user_defined_program_name_.empty() ? user_defined_program_name_ : shader_base_name_;
        for (auto& c : program_label) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                c = '_';
            }
        }
        std::string cache_key = program_label + "_SV" + std::to_string(variant_index) + "_" + std::to_string(defines_hash);

        // Try cache load
        if (_try_load_program_binary(gl_program, cache_key, freshest_source_timestamp)) {
            continue; // Cache hit — variant done
        }

        // Cache miss — store for batched compilation
        pending_variant_t pending;
        pending.variant_index = variant_index;
        pending.program = gl_program;
        pending.cache_key = cache_key;
        pending.freshest_source_timestamp = freshest_source_timestamp;
        pending.resolved_sources = std::move(resolved_sources);
        cache_miss_variants.push_back(std::move(pending));
    }

    // ---- Pass 2: Submit all compilations for cache-miss variants (non-blocking) ----
    for (auto& pending : cache_miss_variants) {
        for (size_t shader_idx = 0; shader_idx < shaders_.size(); ++shader_idx) {
            auto const& shader_file = shaders_[shader_idx];
            shaderc_shader_kind shaderc_shader_kind_to_compile = map_own_shadertype_to_shaderc_kind(shader_file.first);

            auto const& blacklist_it = shader_variant_component_blacklist_.find(pending.variant_index);
            if (blacklist_it != shader_variant_component_blacklist_.end()) {
                if (blacklist_it->second.count(shaderc_shader_kind_to_compile) > 0) {
                    std::cout << "Skipping task shader " << '\n';
                    continue;
                }
            }

            std::string& shader_source_code = pending.resolved_sources[shader_idx];

            GLuint current_gl_shader = glCreateShader((GLenum)(shader_file.first));

            std::string const unique_shader_identifier = shader_file.second + "_" + std::to_string(pending.variant_index);
            std::string shader_filename = std::filesystem::path(unique_shader_identifier).filename().string();
            create_resolved_debug_files(unique_shader_identifier, shader_filename, "./GL_glsl_input/", ".pre_glsl", shader_source_code, cache_shader_files_);

            const char* shader_source_as_c_string = shader_source_code.c_str();
            const int shader_source_length = int(shader_source_code.size());
            glShaderSource(current_gl_shader, 1, &shader_source_as_c_string, &shader_source_length);
            glCompileShader(current_gl_shader); // Non-blocking with ARB_parallel_shader_compile

            pending.shader_handles.push_back(current_gl_shader);
            pending.shader_source_indices.push_back(shader_idx);
        }
    }

    // ---- Pass 3: Harvest compilation results, attach, link, save cache ----
    for (auto& pending : cache_miss_variants) {
        bool compilation_failed = false;

        for (size_t i = 0; i < pending.shader_handles.size(); ++i) {
            GLuint shader = pending.shader_handles[i];
            size_t shader_idx = pending.shader_source_indices[i];

            GLint shader_compilation_success = GL_TRUE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compilation_success); // Blocks until this shader is done

            if (GL_FALSE == shader_compilation_success) {
                GLint shader_info_log_length = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &shader_info_log_length);
                char* shader_info_log = new char[shader_info_log_length];
                glGetShaderInfoLog(shader, shader_info_log_length, NULL, shader_info_log);
                std::cout << '\n';
                std::cout << "\x1B[93m";
                std::cout << "Failed to compile shader: " << shaders_[shader_idx].second << '\n';
                std::cout << "\x1B[93m=========================";
                std::cout << "\033[0m" << '\n';
                std::cout << shader_info_log << '\n';
                delete[] shader_info_log;
                compilation_failed = true;
                break;
            }

            glAttachShader(pending.program, shader);
        }

        if (compilation_failed) {
            for (GLuint shader : pending.shader_handles) {
                glDeleteShader(shader);
            }
            return false;
        }

        glLinkProgram(pending.program);

        // Detach and delete shaders
        for (GLuint shader : pending.shader_handles) {
            glDetachShader(pending.program, shader);
            glDeleteShader(shader);
        }

        GLint program_linking_success = GL_TRUE;
        glGetProgramiv(pending.program, GL_LINK_STATUS, &program_linking_success);

        if (GL_FALSE == program_linking_success) {
            std::cout << "Failed to link program!" << '\n';
            GLint program_info_log_length = 0;
            glGetProgramiv(pending.program, GL_INFO_LOG_LENGTH, &program_info_log_length);
            char* program_info_log = new char[program_info_log_length];
            glGetProgramInfoLog(pending.program, program_info_log_length, NULL, program_info_log);
            std::cout << '\n';
            std::cout << "\x1B[93m";
            std::cout << "Failed to link shader program for shaders: " << '\n';
            for (auto const& shader_file : shaders_) {
                std::cout << "\t" << shader_file.second << '\n';
            }
            std::cout << "\x1B[93m==================================";
            std::cout << "\033[0m" << '\n';
            std::cout << program_info_log << '\n';
            delete[] program_info_log;
            return false;
        }

        // Save binary cache after successful link
        if (enable_binary_cache_) {
            _save_program_binary(pending.program, pending.cache_key, pending.freshest_source_timestamp);
        }
    }

    return true;
}

void shader_program::compile_programs_batched(std::vector<std::shared_ptr<shader_program>> const& programs) {
    // Cross-program batched compilation: submit ALL shader compilations from ALL programs
    // before checking any results, maximizing the driver's parallel compilation window.

    struct pending_shader_t {
        GLuint shader_handle;
        size_t program_idx;   // index into programs vector
        int32_t variant_index;
        size_t shader_file_idx; // index into shaders_
    };

    struct pending_variant_t {
        size_t program_idx;
        int32_t variant_index;
        uint32_t gl_program;
        std::string cache_key;
        std::filesystem::file_time_type freshest_source_timestamp;
        std::vector<std::string> resolved_sources;
        std::vector<size_t> pending_shader_start; // start index in all_pending_shaders
        size_t pending_shader_count = 0;
    };

    std::vector<pending_shader_t> all_pending_shaders;
    std::vector<pending_variant_t> all_pending_variants;

    // ---- Pass 1: Resolve sources and attempt cache loads for ALL programs ----
    for (size_t prog_idx = 0; prog_idx < programs.size(); ++prog_idx) {
        auto& prog = *programs[prog_idx];

        for (auto& [variant_index, gl_program] : prog.shader_program_variants_) {
            gl_program = glCreateProgram();
            glProgramParameteri(gl_program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

            if (!enable_binary_cache_) {
                pending_variant_t pending;
                pending.program_idx = prog_idx;
                pending.variant_index = variant_index;
                pending.gl_program = gl_program;
                pending.resolved_sources.reserve(prog.shaders_.size());
                for (auto const& shader_file : prog.shaders_) {
                    std::filesystem::file_time_type ts = std::filesystem::file_time_type::clock::time_point();
                    pending.resolved_sources.push_back(prog._resolve_shader_source_code(shader_file.second, variant_index, ts));
                }
                all_pending_variants.push_back(std::move(pending));
                continue;
            }

            // Resolve all sources and find freshest timestamp
            std::filesystem::file_time_type freshest_source_timestamp = std::filesystem::file_time_type::clock::time_point();
            std::vector<std::string> resolved_sources;
            resolved_sources.reserve(prog.shaders_.size());
            for (auto const& shader_file : prog.shaders_) {
                std::filesystem::file_time_type component_timestamp = std::filesystem::file_time_type::clock::time_point();
                resolved_sources.push_back(prog._resolve_shader_source_code(shader_file.second, variant_index, component_timestamp));
                if (component_timestamp > freshest_source_timestamp) {
                    freshest_source_timestamp = component_timestamp;
                }
            }

            // Build cache key
            std::string defines_to_hash;
            for (auto const& d : static_defines_to_inject_into_each_shader) { defines_to_hash += d; }
            for (auto const& d : prog.shader_variant_defines_[variant_index]) { defines_to_hash += d; }
            for (auto const& d : prog.program_local_defines_) { defines_to_hash += d; }
            if (prog.last_used_hot_plugged_local_shader_variant_program_defines_.contains(variant_index)) {
                for (auto const& d : prog.last_used_hot_plugged_local_shader_variant_program_defines_[variant_index]) { defines_to_hash += d; }
            }
            size_t defines_hash = std::hash<std::string>{}(defines_to_hash);

            std::string program_label = !prog.user_defined_program_name_.empty() ? prog.user_defined_program_name_ : prog.shader_base_name_;
            for (auto& c : program_label) {
                if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                    c = '_';
                }
            }
            std::string cache_key = program_label + "_SV" + std::to_string(variant_index) + "_" + std::to_string(defines_hash);

            // Try cache load
            if (prog._try_load_program_binary(gl_program, cache_key, freshest_source_timestamp)) {
                continue; // Cache hit
            }

            // Cache miss
            pending_variant_t pending;
            pending.program_idx = prog_idx;
            pending.variant_index = variant_index;
            pending.gl_program = gl_program;
            pending.cache_key = cache_key;
            pending.freshest_source_timestamp = freshest_source_timestamp;
            pending.resolved_sources = std::move(resolved_sources);
            all_pending_variants.push_back(std::move(pending));
        }
    }

    // ---- Pass 2: Submit ALL compilations across ALL programs (non-blocking) ----
    for (auto& pending : all_pending_variants) {
        auto& prog = *programs[pending.program_idx];
        size_t start_idx = all_pending_shaders.size();

        for (size_t shader_idx = 0; shader_idx < prog.shaders_.size(); ++shader_idx) {
            auto const& shader_file = prog.shaders_[shader_idx];
            shaderc_shader_kind shaderc_shader_kind_to_compile = map_own_shadertype_to_shaderc_kind(shader_file.first);

            auto const& blacklist_it = prog.shader_variant_component_blacklist_.find(pending.variant_index);
            if (blacklist_it != prog.shader_variant_component_blacklist_.end()) {
                if (blacklist_it->second.count(shaderc_shader_kind_to_compile) > 0) {
                    continue;
                }
            }

            std::string& shader_source_code = pending.resolved_sources[shader_idx];

            GLuint current_gl_shader = glCreateShader((GLenum)(shader_file.first));

            std::string const unique_shader_identifier = shader_file.second + "_" + std::to_string(pending.variant_index);
            std::string shader_filename = std::filesystem::path(unique_shader_identifier).filename().string();
            create_resolved_debug_files(unique_shader_identifier, shader_filename, "./GL_glsl_input/", ".pre_glsl", shader_source_code, prog.cache_shader_files_);

            const char* shader_source_as_c_string = shader_source_code.c_str();
            const int shader_source_length = int(shader_source_code.size());
            glShaderSource(current_gl_shader, 1, &shader_source_as_c_string, &shader_source_length);
            glCompileShader(current_gl_shader); // Non-blocking with ARB_parallel_shader_compile

            pending_shader_t ps;
            ps.shader_handle = current_gl_shader;
            ps.program_idx = pending.program_idx;
            ps.variant_index = pending.variant_index;
            ps.shader_file_idx = shader_idx;
            all_pending_shaders.push_back(ps);
        }

        pending.pending_shader_start.push_back(start_idx);
        pending.pending_shader_count = all_pending_shaders.size() - start_idx;
    }

    std::cout << "Batched shader compilation: " << all_pending_shaders.size() << " shaders across "
              << all_pending_variants.size() << " cache-miss variants submitted" << '\n';

    // ---- Pass 3: Harvest results, attach, link, save cache ----
    for (auto& pending : all_pending_variants) {
        auto& prog = *programs[pending.program_idx];
        bool compilation_failed = false;

        size_t start = pending.pending_shader_start[0];
        for (size_t i = 0; i < pending.pending_shader_count; ++i) {
            auto& ps = all_pending_shaders[start + i];

            GLint shader_compilation_success = GL_TRUE;
            glGetShaderiv(ps.shader_handle, GL_COMPILE_STATUS, &shader_compilation_success);

            if (GL_FALSE == shader_compilation_success) {
                GLint shader_info_log_length = 0;
                glGetShaderiv(ps.shader_handle, GL_INFO_LOG_LENGTH, &shader_info_log_length);
                char* shader_info_log = new char[shader_info_log_length];
                glGetShaderInfoLog(ps.shader_handle, shader_info_log_length, NULL, shader_info_log);
                std::cout << '\n';
                std::cout << "\x1B[93m";
                std::cout << "Failed to compile shader: " << prog.shaders_[ps.shader_file_idx].second << '\n';
                std::cout << "\x1B[93m=========================";
                std::cout << "\033[0m" << '\n';
                std::cout << shader_info_log << '\n';
                delete[] shader_info_log;
                compilation_failed = true;
                break;
            }

            glAttachShader(pending.gl_program, ps.shader_handle);
        }

        if (compilation_failed) {
            for (size_t i = 0; i < pending.pending_shader_count; ++i) {
                glDeleteShader(all_pending_shaders[start + i].shader_handle);
            }
            std::cout << "Shader compilation failed for program: " << prog.user_defined_program_name_ << '\n';
            exit(-1);
        }

        glLinkProgram(pending.gl_program);

        // Detach and delete shaders after linking
        for (size_t i = 0; i < pending.pending_shader_count; ++i) {
            auto& ps = all_pending_shaders[start + i];
            glDetachShader(pending.gl_program, ps.shader_handle);
            glDeleteShader(ps.shader_handle);
        }

        GLint program_linking_success = GL_TRUE;
        glGetProgramiv(pending.gl_program, GL_LINK_STATUS, &program_linking_success);

        if (GL_FALSE == program_linking_success) {
            GLint program_info_log_length = 0;
            glGetProgramiv(pending.gl_program, GL_INFO_LOG_LENGTH, &program_info_log_length);
            char* program_info_log = new char[program_info_log_length];
            glGetProgramInfoLog(pending.gl_program, program_info_log_length, NULL, program_info_log);
            std::cout << '\n';
            std::cout << "\x1B[93m";
            std::cout << "Failed to link shader program for shaders: " << '\n';
            for (auto const& shader_file : prog.shaders_) {
                std::cout << "\t" << shader_file.second << '\n';
            }
            std::cout << "\x1B[93m==================================";
            std::cout << "\033[0m" << '\n';
            std::cout << program_info_log << '\n';
            delete[] program_info_log;
            exit(-1);
        }

        // Save binary cache after successful link
        if (enable_binary_cache_) {
            prog._save_program_binary(pending.gl_program, pending.cache_key, pending.freshest_source_timestamp);
        }
    }

    std::cout << "Batched shader compilation complete" << '\n';
}

void shader_program::register_shader_definitions(std::vector<std::string> const& definitions_to_add_after_first_line) {
    static_defines_to_inject_into_each_shader = definitions_to_add_after_first_line;
}

void shader_program::add_to_existing_shader_definitions(std::vector<std::string> const& definitions_to_add_after_first_line) {
    static_defines_to_inject_into_each_shader.insert(static_defines_to_inject_into_each_shader.end(), definitions_to_add_after_first_line.begin(), definitions_to_add_after_first_line.end());
}

}
