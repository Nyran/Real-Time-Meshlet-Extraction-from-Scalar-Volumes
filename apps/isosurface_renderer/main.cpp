// =====================================================================
// Where is the core method from the paper implemented?
//
//   Region analysis (task shader, paper Sec. 3.2.1, supplementary Listing 1):
//     glsl/task_mesh_shader_based_isosurface_extraction/region_size_analysis.task
//
//   Meshlet extraction (mesh shader, paper Sec. 3.2.2, supplementary Listing 2):
//     glsl/task_mesh_shader_based_isosurface_extraction/isosurface_extraction.mesh
//
//   Pipeline wiring (which shaders are linked into the task+mesh program):
//     framework/base_renderer_create_shader_programs.cpp
//
// To run the proposed task+mesh-shader extraction pipeline, pass
// --task_mesh_persistent. See README.md for the full CLI surface.
// =====================================================================

#include <framework_cmake_config.hpp>


#include "isosurface_renderer_app.hpp"

#include "../../framework/shader_program.hpp"



#include <glm/gtx/string_cast.hpp>

//external library: simple, yet powerful header only command line argument parser:
#include <CLI11.hpp>

#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>


//default parameters
std::string in_volume_file_path;

float viewport_scaling = 1.0f;
bool start_viewport_scaling_evaluaton = false;

int persistent_extraction_block_size_x = 0;
int persistent_extraction_block_size_y = 0;
int persistent_extraction_block_size_z = 0;

glm::ivec2 const base_resolution = { 1920, 1080 };

bool set_no_gui = false;

std::unique_ptr<IsosurfaceRendererApp> app_ptr = nullptr;

CLI::App app_cmd_line_parser{ "Volume Rendering through on-the-fly geometry extraction" };

IsosurfaceRendererApp::AppRunDescriptor run_descriptor;

initial_rendering_mode_settings_t  initial_rendering_mode_settings;


void define_command_line_arguments() {
    // main CLI arguments
    app_cmd_line_parser.add_option("file,-f,--file", in_volume_file_path, "Volume file to to visualize, containing the tokens _w _h _d _c and _b.")->required()->ignore_case()->ignore_underscore();

    app_cmd_line_parser.add_option("-s,--viewport_scaling", viewport_scaling, "Value in (0.0,1.0], defining a scale relative to the native resolution 3840x2160 pixels.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_flag("--evaluation_mode,--eval", run_descriptor.is_in_evaluation_mode, "Start performance measurement using different viewport sizes.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_flag("--log_memory_footprint,--memory_footprint", run_descriptor.eval_settings.log_memory_footprint_after_extraction, "Log Memory footprint after extraction (involves CPU-GPU interaction).")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_flag("--per_memory_footprint_component_eval,--memory_component_eval", run_descriptor.eval_settings.evaluate_individual_pipeline_memory_components, "Set this flag to meaure the memory footprint components of the persistent extractions individually.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_flag("--per_timing_component_evaluation,--timing_component_eval,--per_time_component_evaluation,--time_component_eval", run_descriptor.eval_settings.evaluate_individual_pipeline_timing_components, "Set this flag to measure the runtime of individual timing components.")->ignore_case()->ignore_underscore();

    app_cmd_line_parser.add_flag("--task_shader_based_extraction,--task_mesh_persistent", initial_rendering_mode_settings.use_task_mesh_shader_pipeline_for_persistent_extraction, "Use Task and Mesh Shaders for persistent extraction.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_flag("--perform_reference_meshlet_construction,--reference_meshlets,--offline_meshlets", initial_rendering_mode_settings.perform_reference_meshlet_construction, "Construct reference meshlet using meshoptimizer (by Arseny Kapoulkine)")->ignore_case()->ignore_underscore();


    app_cmd_line_parser.add_option("-i,--iso_value,--start_iso_value", run_descriptor.start_iso_value, "Iso value to start the visualization with. Default: 0.2.")->ignore_case()->ignore_underscore();
    
    // Default matches the configuration reported throughout the paper's evaluation (C = 128).
    app_cmd_line_parser.add_option("--max_PMB_cells,--max_PMB_cells_per_mesh_wg", run_descriptor.eval_settings.num_PMB_cells_allowed_per_mesh_shading_workgroup)->ignore_case()->ignore_underscore()->default_val(128);

    app_cmd_line_parser.add_flag("--precompile_shaders, --pre_compile_shaders", run_descriptor.precompile_shaders)->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_flag("--no_shader_cache,--disable_shader_cache", run_descriptor.disable_shader_cache, "Disable GL program binary caching")->ignore_case()->ignore_underscore();


    /*
        iso value range evaluation related >
    */
    app_cmd_line_parser.add_option("--end_iso_value, --end_iso_val", run_descriptor.eval_settings.end_iso_value, "Iso value to define an end to the range for dynamic iso value visualizations. Default: FLT_MAX to define a non-existent range.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_flag("--range_eval,--perform_isovalue_range_evaluation", run_descriptor.eval_settings.perform_iso_range_evaluation, "Perform iso value range evaluation.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("--step_size, --range_step_size, --iso_step_size", run_descriptor.eval_settings.iso_range_evalation_step_size, "Magnitude for step size for increments between start and end for range value evaluation")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("--eval_samples,--evaluation_samples", run_descriptor.eval_settings.evaluation_samples_per_measurement, "Num samples to take individual evaluation configuration.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("--eval_base_name, --base_name, --basename, --name", run_descriptor.eval_settings.evaluation_base_name, "Base name that is the prefix for all the log files.")->ignore_case()->ignore_underscore();
    /* < */


    app_cmd_line_parser.add_option("-b,--base_color", run_descriptor.shading_base_color, "Base color for shading, rgba")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("-w,--wireframe_base_color", run_descriptor.wireframe_base_color, "Base color for wire frame rendering, rgba")->ignore_case()->ignore_underscore();

    app_cmd_line_parser.add_option("--swap_interval,--swap,--vsync", run_descriptor.swap_interval)->ignore_case()->ignore_underscore();

    app_cmd_line_parser.add_option("-X,--persistent_block_size_x", persistent_extraction_block_size_x, "block size for persistent extraction dim X")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("-Y,--persistent_block_size_y", persistent_extraction_block_size_y, "block size for persistent extraction dim Y")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("-Z,--persistent_block_size_z", persistent_extraction_block_size_z, "block size for persistent extraction dim Z")->ignore_case()->ignore_underscore();

    app_cmd_line_parser.add_flag("--always_reextract,--reextract_always,--reextract", run_descriptor.eval_settings.always_reextract, "Reextract in every frame")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_flag("--no_gui_startup,--no_gui", set_no_gui, "If set, prevents the ImGui to be initialized")->ignore_case()->ignore_underscore()->default_val(false);

    

    app_cmd_line_parser.add_option("--m_yaw,--model_yaw", run_descriptor.initial_model_transform_settings.yaw, "Initial 'Yaw' rotation angle in degree for active model (application order: YAW->pitch->roll)")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("--m_pitch,--model_pitch", run_descriptor.initial_model_transform_settings.pitch, "Initial 'Pitch' rotation angle in degree for active model (application order: yaw->PITCH->roll)")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("--m_roll,--model_roll", run_descriptor.initial_model_transform_settings.roll, "Initial 'Roll' rotation angle in degree for active model (application order: yaw->pitch->ROLL)")->ignore_case()->ignore_underscore();

    app_cmd_line_parser.add_option("--index_buffer_size_in_MB,--index_buffer_MB", run_descriptor.geom_buffer_settings.index_buffer_size_in_MB, "Explicitly define the maximum size of the extracted index buffer in Megabyte to avoid using other heuristics.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("--position_buffer_size_in_MB,--position_buffer_MB", run_descriptor.geom_buffer_settings.position_buffer_size_in_MB, "Explicitly define the maximum size of the extracted vertex position buffer in Megabyte to avoid using other heuristics.")->ignore_case()->ignore_underscore();
    app_cmd_line_parser.add_option("--normal_buffer_size_in_MB,--normal_buffer_MB", run_descriptor.geom_buffer_settings.normal_buffer_size_in_MB, "Explicitly define the maximum size of the extracted vertex normal buffer in Megabyte to avoid using other heuristics.")->ignore_case()->ignore_underscore();
}


void configure_initial_rendering_mode() {
    if (set_no_gui) {
        framework::BaseRenderer::set_NO_GUI(true);
    }

}

#ifdef _WIN32
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

void apply_parameter_dependent_behaviour() {

    configure_initial_rendering_mode();
    

    if ( ((run_descriptor.eval_settings.evaluate_individual_pipeline_timing_components) || (run_descriptor.eval_settings.evaluate_individual_pipeline_memory_components))) {
        run_descriptor.is_in_evaluation_mode = true;
    }

    if (run_descriptor.eval_settings.evaluate_individual_pipeline_memory_components) {
        run_descriptor.eval_settings.log_memory_footprint_after_extraction = true;
    }

    if (run_descriptor.is_in_evaluation_mode) {
        std::filesystem::path base_path(run_descriptor.eval_settings.evaluation_base_name);
        run_descriptor.eval_settings.evaluation_base_path = base_path.parent_path().string() + "/";
        run_descriptor.eval_settings.evaluation_base_name = base_path.relative_path().filename().string();
    }
   

    if (start_viewport_scaling_evaluaton) {
        std::cout << "Started viewport evaluation" << '\n';
    }

    framework::shader_program::add_to_existing_shader_definitions({ "#extension GL_NV_gpu_shader5 : enable" });


    run_descriptor.window_resolution = glm::ivec2(viewport_scaling * base_resolution[0], viewport_scaling * base_resolution[1]);
    //

    if (run_descriptor.eval_settings.pass_extraction_block_ids_as_implicit_AABBs) {
        run_descriptor.eval_settings.extract_bounding_volumes = true;

        framework::shader_program::add_to_existing_shader_definitions({ "#define SHADER_VARIANT__EXTRACT_BOUNDING_VOLUMES" });
        framework::shader_program::add_to_existing_shader_definitions({ "#define PASS_THROUGH_EXTRACTION_BLOCK_IDS" });
    }




  

    if (0 == persistent_extraction_block_size_x) {
        persistent_extraction_block_size_x = 8;
    }

    if (0 == persistent_extraction_block_size_y) {
        persistent_extraction_block_size_y = 8;
    }

    if (0 == persistent_extraction_block_size_z) {
        persistent_extraction_block_size_z = 4;
    }


    if (
        !((persistent_extraction_block_size_x > 0) && (std::has_single_bit<uint32_t>(persistent_extraction_block_size_x)))
        ) {
        std::cout << "Unpadded block size X is not a valid power of 2. Exiting." << '\n';
        std::exit(-1);
    }
   
    if (
        !((persistent_extraction_block_size_y > 0) && (std::has_single_bit<uint32_t>(persistent_extraction_block_size_y)))
        ) {
        std::cout << "Unpadded block size Y is not a valid power of 2. Exiting." << '\n';
        std::exit(-2);
    }

    if (
        !((persistent_extraction_block_size_z > 0) && (std::has_single_bit<uint32_t>(persistent_extraction_block_size_z)))
        ) {
        std::cout << "Unpadded block size Z is not a valid power of 2. Exiting." << '\n';
        std::exit(-3);
    }


    run_descriptor.eval_settings.extraction_block_sizes_for_labeling = { persistent_extraction_block_size_x , persistent_extraction_block_size_y , persistent_extraction_block_size_z };




    run_descriptor.window_resolution = glm::ivec2(viewport_scaling * base_resolution[0], viewport_scaling * base_resolution[1]);
    //
    app_ptr = std::make_unique<IsosurfaceRendererApp>(run_descriptor) ;


    // Block-size defines are now injected as program-local defines in
    // create_persistent_isosurface_extraction_shader_programs_() and opengl_volume_gpu_upload.cpp,
    // so that changing block size only invalidates the affected shader programs' caches.
    framework::general_rendering_settings::get_instance()->PERSISTENT_EXTRACTION_BLOCK_SIZE = { persistent_extraction_block_size_x , persistent_extraction_block_size_y, persistent_extraction_block_size_z };
}

void print_useful_hotkeys() {
    std::cout << "////////////////////////////////////////////////////////////////////////////////////////////////////" << '\n';
    std::cout << "//  Research framework for the EGPGV 2026 paper Real-Time Meshlet Extraction from Scalar Volumes  //" << '\n';
    std::cout << "////////////////////////////////////////////////////////////////////////////////////////////////////" << '\n';
    std::cout << "/                                                                                                  /" << '\n';
    std::cout << "/                                         Useful hotkeys:                                          /" << '\n';
    std::cout << "/--------------------------------------------------------------------------------------------------/" << '\n';
    std::cout << "/                                  c     - open camera settings                                    /" << '\n';
    std::cout << "/                                  r     - toggle volume auto rotation                             /" << '\n';
    std::cout << "/                                                                                                  /" << '\n';
    std::cout << "////////////////////////////////////////////////////////////////////////////////////////////////////" << '\n';
    std::cout << '\n';

    std::cout << '\n';
    std::cout << "For more information, please refer to the README.md file" << '\n' << '\n';
}

int main(int argc, char** argv)
{
    std::cout << "passed through arguments: " << '\n';
    for (int arg_idx = 0; arg_idx < argc; ++arg_idx) {
        std::cout << argv[arg_idx] << " ";
    }

    std::cout << '\n';

    print_useful_hotkeys();

    // first we define all parameters
    define_command_line_arguments();
    // then we use the parsing macro from CLI11 ...
    CLI11_PARSE(app_cmd_line_parser, argc, argv);
    // ... afterwards we obtained the parameters
    apply_parameter_dependent_behaviour();

    app_ptr->initialize_framework(run_descriptor);
    app_ptr->load_volumes(in_volume_file_path);
    app_ptr->create_shader_programs();
    app_ptr->start_render_loop(initial_rendering_mode_settings);
    app_ptr->tear_down_framework();


    return 0;
}