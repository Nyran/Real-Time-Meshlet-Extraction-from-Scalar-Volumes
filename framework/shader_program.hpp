#ifndef FRAMEWORK_SHADER_PROGRAM_HPP
#define FRAMEWORK_SHADER_PROGRAM_HPP


#include <framework_cmake_config.hpp>

#include "framework_platform.hpp"

// framework internal
#include "shader_types.hpp"

// externals: glm
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderc/shaderc.hpp"

// C++ standard libraries
#include <filesystem>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace framework {

using associated_shaders_per_variant_t = std::map<int32_t, std::vector<associated_shader_code_t>> ;
using shader_variant_defines_t = std::map<int32_t, std::set<std::string>>;
using shader_variant_component_blacklist_t = std::map<int32_t, std::set<shaderc_shader_kind>>;

struct shader_paths_and_tags {
    std::string path;
    std::string optional_tag;
};
// use named parameters
struct shader_components {
  //turing and above only
   shader_paths_and_tags task = {};
   shader_paths_and_tags mesh = {};

  //more classic shader stages
   shader_paths_and_tags vertex = {};
   shader_paths_and_tags tesselation_control = {};
   shader_paths_and_tags tesselation_evaluation = {};
   shader_paths_and_tags geometry = {};
   shader_paths_and_tags fragment = {};
   shader_paths_and_tags compute = {};

  std::string program_name;
};

int32_t constexpr DEFAULT_SHADER_INDEX = 0;// std::numeric_limits<int32_t>::min();

int32_t constexpr TASK_SHADER_EXT_VARIANT_WG_SIZE_32_INDEX = 0; // default
int32_t constexpr TASK_SHADER_EXT_VARIANT_WG_SIZE_64_INDEX = std::numeric_limits<int32_t>::max() >> 7;
int32_t constexpr TASK_SHADER_EXT_VARIANT_WG_SIZE_96_INDEX = std::numeric_limits<int32_t>::max() >> 6;
int32_t constexpr TASK_SHADER_EXT_VARIANT_WG_SIZE_128_INDEX = std::numeric_limits<int32_t>::max() >> 5;

int32_t constexpr MESH_SHADER_EXT_VARIANT_WG_SIZE_32_INDEX = 0; // default
int32_t constexpr MESH_SHADER_EXT_VARIANT_WG_SIZE_64_INDEX = std::numeric_limits<int32_t>::max() >> 4;
int32_t constexpr MESH_SHADER_EXT_VARIANT_WG_SIZE_96_INDEX = std::numeric_limits<int32_t>::max() >> 3;
int32_t constexpr MESH_SHADER_EXT_VARIANT_WG_SIZE_128_INDEX = std::numeric_limits<int32_t>::max() >> 2;
int32_t constexpr MESH_SHADER_EXT_VARIANT_ADD_IN_INDEX = std::numeric_limits<int32_t>::max() >> 1;

//create variants containing 0 to 
std::vector<std::string> const task_shader_workgroup_size_variants = { "#define EXT_TASK_SHADER_WG_SIZE 32" };
std::vector<std::string> const  mesh_shader_workgroup_size_variants = { "#define EXT_MESH_SHADER_WG_SIZE 32" };

std::map<std::string, int32_t> const workgroup_size_to_shader_variant_index_map{
    {"#define EXT_TASK_SHADER_WG_SIZE 32", TASK_SHADER_EXT_VARIANT_WG_SIZE_32_INDEX},

    {"#define EXT_MESH_SHADER_WG_SIZE 32", MESH_SHADER_EXT_VARIANT_WG_SIZE_32_INDEX},
};


class FRAMEWORK_DLL_EXPORT shader_program {
  public:
    shader_program(bool compile_shader_variants_immediately, shader_components const& components, shader_variant_defines_t const& shader_variant_defines = {}, std::set<std::string> const& program_local_defines = {}, bool cache_compiled_shaders = true, shader_variant_component_blacklist_t const& shader_variant_component_blacklist = {} );

    void use(int32_t shader_variant_index = DEFAULT_SHADER_INDEX);

    // wrapper for compute shader dispatch function
    void dispatch(glm::uvec3 const& num_workgroups_3d);

    void dispatch(uint32_t workgroups_x, uint32_t workgroups_y, uint32_t workgroups_z);
    // wrapper for compute shader dispatch indirect function
    void dispatch_indirect(uint32_t indirect_buffer_byte_offset);

    // template functions to facilitate uniform uploads, see cpp-file
    template <typename UNIFORM_TYPE>
    void upload_uniform(std::string const& uniform_name, UNIFORM_TYPE const& uniform, int32_t shader_variant_index = DEFAULT_SHADER_INDEX);
    
    bool compile_shaders() { return _compile_shaders(); }

    // Batch-compile multiple shader programs at once, maximizing driver-side parallel compilation.
    // All programs should have been constructed with compile_shader_variants_immediately = false.
    static void compile_programs_batched(std::vector<std::shared_ptr<shader_program>> const& programs);

    // convenience to dynamically inject shader constants (preprocessor #defines added after the #version line)
    static void register_shader_definitions(std::vector<std::string> const& definitions_to_add_after_first_line);
    /* {
        static_defines_to_inject_into_each_shader = definitions_to_add_after_first_line;
    }*/

    static void add_to_existing_shader_definitions(std::vector<std::string> const& definitions_to_add_after_first_line);

    static void set_binary_cache_enabled(bool enabled);
    /* {
        static_defines_to_inject_into_each_shader.insert(static_defines_to_inject_into_each_shader.end(), definitions_to_add_after_first_line.begin(), definitions_to_add_after_first_line.end());
    }*/

    //retrieve all the named and modified shader components for a given variant index
    std::vector<associated_shader_code_t> get_associated_shader_code_for_variant(int32_t shader_variant_index) const;

    /*
    void hotplug_local_shader_variant_defines_and_recompile(
        int32_t shader_variant_index, 
        std::vector<std::string> const& local_shader_variant_program_defines_to_hot_plug = {});
    */
  private:
    static std::vector<std::string> static_defines_to_inject_into_each_shader;

    void _check_whether_shader_variant_exists(int32_t shader_variant_index, std::string const& uniform_name) const;
    void _print_shader_variant_error_message(int32_t shader_variant_index, std::string const& uniform_name) const;

    bool _compile_shader_variant(int32_t shader_variant_index);
    bool _compile_shaders();
    bool _compile_shader_program_variant_OpenGL_path(std::pair < int32_t const, uint32_t>& shader_program_variant_and_program_pair);

    std::string _resolve_shader_source_code(std::string const& shader_file_path, int32_t shader_program_variant_index, std::filesystem::file_time_type& freshest_encountered_file_timestamp);

    bool _read_shader_unmodified(std::string const& shader_file_path, std::string& shader_source_out);

    std::string shader_base_name_;
    std::string user_defined_program_name_;

    shader_variant_defines_t shader_variant_defines_;
    shader_variant_component_blacklist_t shader_variant_component_blacklist_;
    std::set<std::string> program_local_defines_;
    std::map<int32_t, std::set<std::string>> last_used_hot_plugged_local_shader_variant_program_defines_;
    std::vector<associated_shader_code_t> shaders_;
    std::map<int32_t, uint32_t> shader_program_variants_;

    bool cache_shader_files_;
    bool pre_compile_shader_programs_;

    static std::string shader_binary_cache_directory_;
    static bool enable_binary_cache_;

    bool _try_load_program_binary(uint32_t program, std::string const& cache_key, std::filesystem::file_time_type source_timestamp);
    void _save_program_binary(uint32_t program, std::string const& cache_key, std::filesystem::file_time_type source_timestamp);

    //vector keeping the original source code for analysis purposes per shader variant
    associated_shaders_per_variant_t ASCII_individual_shaders_per_shader_variant_;
};

}

#endif //FRAMEWORK_SHADER_PROGRAM_HPP