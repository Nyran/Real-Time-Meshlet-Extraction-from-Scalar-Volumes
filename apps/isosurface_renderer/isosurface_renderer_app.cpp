// framework internal

#include <framework_cmake_config.hpp>

#include "isosurface_renderer_app.hpp"

#include "framework/OpenGL_impl/opengl_volume.hpp"

#include <framework/OpenGL_impl/opengl_renderer.hpp>


#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#else
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif


// external: gl related
#include <GL/glcorearb.h>


#include <glm/gtx/string_cast.hpp>

// C++ standard libraries
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <thread>


std::unique_ptr<framework::BaseRenderer> IsosurfaceRendererApp::renderer_ = nullptr;

bool IsosurfaceRendererApp::draw_gui_ = true;

using accumulated_global_frame_times_t = std::pair<int32_t, double>;
std::map<float, accumulated_global_frame_times_t> global_frame_times_per_iso_value;

struct relevant_per_component_time_attributes {
    double occupancy_filtering = 0.0f;
    double extraction = 0.0;
    double rendering = 0.0;
};

using accumulated_global_memory_footprint_t = std::pair<int32_t, uint64_t>;
std::map<float, accumulated_global_memory_footprint_t> global_frame_memory_footprint_per_iso_value;

struct relevant_per_component_memory_footprint_attributes {
    uint32_t total_num_extraction_blocks = 0;
    uint32_t total_num_geometry_patches = 0;
    
    uint32_t total_num_triangles = 0;
    uint32_t total_num_vertices = 0;

    uint32_t num_byte_for_vertex_position = 0;
    uint32_t num_byte_for_vertex_normal = 0;
    uint32_t num_byte_for_vertex_indices = 0;

    uint32_t num_byte_for_block_draw_descriptor_persistent_part = 0;
    uint32_t num_byte_for_block_draw_descriptor_live_part = 0;
    uint32_t num_byte_for_meshlet_draw_descriptor = 0;
    uint32_t num_byte_for_culling_info = 0;

    uint32_t total_num_byte = 0;
};

using accumulated_per_component_frame_times_t = std::pair<int32_t, relevant_per_component_time_attributes>;
using accumulated_per_component_memory_footprint_t = std::pair<int32_t, relevant_per_component_memory_footprint_attributes>;

std::map<float, accumulated_per_component_frame_times_t> per_component_frame_times_per_iso_value;
std::map<float, accumulated_per_component_memory_footprint_t> per_component_memory_footprint_per_iso_value;

// helper function for loading frame buffer extensions
void load_gl_frame_buffer_extensions() {
}

IsosurfaceRendererApp::IsosurfaceRendererApp(AppRunDescriptor const& run_descriptor) : app_descriptor_{run_descriptor} {}


IsosurfaceRendererApp::~IsosurfaceRendererApp() {}

std::string IsosurfaceRendererApp::compile_log_file_name_(std::string const& file_suffix, std::string const& token_to_add = "") {

    //std::string rendering_mode = "indirect"
    std::string rendering_mode_identifier = "__render_mode__";
    std::string indirect_compute_mode_identifier;

    std::string block_size_identifier;

        rendering_mode_identifier += "indirect";

        block_size_identifier = "__BS__" + std::to_string(app_descriptor_.eval_settings.extraction_block_sizes_for_labeling[0]) + "_"
            + std::to_string(app_descriptor_.eval_settings.extraction_block_sizes_for_labeling[1]) + "_"
            + std::to_string(app_descriptor_.eval_settings.extraction_block_sizes_for_labeling[2]);

        switch (renderer_->get_active_indirect_rendering_compute_mode()) {
        case framework::IndirectRenderingComputeMode::COMPUTE_SHADER_BASED_EXTRACTION: {
            indirect_compute_mode_identifier = "__comp_mode__cs__";
            break;
        }
        case framework::IndirectRenderingComputeMode::TASK_MESH_SHADER_BASED_EXTRACTION: {
            indirect_compute_mode_identifier = "__comp_mode__tms__";
            break;
        }
        case framework::IndirectRenderingComputeMode::MESHOPT_OFFLINE_MESHLET_COMPUTATION: {
            indirect_compute_mode_identifier = "__comp_mode__offl__";
        }
        }


    std::string start_iso_value_with_2_decimals = "0.0";
    {
        std::ostringstream start_iso_value_string_stream;
        start_iso_value_string_stream << std::fixed << std::setprecision(2) << (app_descriptor_.start_iso_value);
        start_iso_value_with_2_decimals = start_iso_value_string_stream.str();
    }


    std::string end_iso_value_with_2_decimals = "1.0";
    {
        std::ostringstream end_iso_value_string_stream;
        end_iso_value_string_stream << std::fixed << std::setprecision(2) << (app_descriptor_.eval_settings.end_iso_value);
        end_iso_value_with_2_decimals = end_iso_value_string_stream.str();
    }

    std::string iso_value_identifier = "__iso_mode__";
    if (app_descriptor_.eval_settings.perform_iso_range_evaluation) {
        iso_value_identifier += "d_" + start_iso_value_with_2_decimals + "_to_" + (end_iso_value_with_2_decimals);
    }
    else {
        iso_value_identifier += "s_" + start_iso_value_with_2_decimals;
    }

    std::filesystem::create_directories(app_descriptor_.eval_settings.evaluation_base_path);

    std::string model_name = app_descriptor_.eval_settings.evaluation_base_path + "__vol__" + app_descriptor_.eval_settings.evaluation_base_name;

    std::string num_allowed_PMB_cells_per_workgroup = "__PMB_CELLS__" + std::to_string(app_descriptor_.eval_settings.num_PMB_cells_allowed_per_mesh_shading_workgroup/32) + "";

    std::string out_log_file_name;

    out_log_file_name
        = model_name
        + "__" + token_to_add 
        + block_size_identifier
        + num_allowed_PMB_cells_per_workgroup
        + rendering_mode_identifier
        + indirect_compute_mode_identifier
        + iso_value_identifier
        + "__"+ file_suffix;


    return out_log_file_name;
}

void IsosurfaceRendererApp::write_global_frame_time_log_file_() {
    std::string const global_timing_out_log_file_name = compile_log_file_name_(".perf_log", "global");
    std::ofstream global_timing_log_file(global_timing_out_log_file_name, std::ios::out | std::ios::binary);

    if (!global_timing_log_file.is_open()) {
        std::cout << "Could not open file: " << global_timing_out_log_file_name << " for writing performance logs" << '\n';
        exit(-1);
    }

    for (auto const& result_entry : global_frame_times_per_iso_value) {
        global_timing_log_file << result_entry.first << "\t" << result_entry.second.second << '\n';
    }

    global_timing_log_file.close();
    
    std::cout << "Wrote global timing log file: " << global_timing_out_log_file_name << '\n';
}

void IsosurfaceRendererApp::write_per_component_frame_time_log_file_() {
    std::string const per_timing_component_timing_log_file_name = compile_log_file_name_(".perf_log", "per_component");
    std::ofstream per_timing_component_out_log_file(per_timing_component_timing_log_file_name, std::ios::out | std::ios::binary);

    if (!per_timing_component_out_log_file.is_open()) {
        std::cout << "Could not open file: " << per_timing_component_timing_log_file_name << " for writing performance logs" << '\n';
        exit(-1);
    }

    for (auto const& result_entry : per_component_frame_times_per_iso_value) {
        per_timing_component_out_log_file << result_entry.first << '\t' << result_entry.second.second.occupancy_filtering << '\t' << result_entry.second.second.extraction << '\t' << result_entry.second.second.rendering << '\n';
    }

    per_timing_component_out_log_file.close();

    std::cout << "Wrote per timing component log file: " << per_timing_component_timing_log_file_name << '\n';
}

void IsosurfaceRendererApp::write_global_memory_footprint_stats_log_file_() {
    std::string const global_memory_footprint_out_log_file_name = compile_log_file_name_(".mem_fp_log", "global");


    std::ofstream global_memory_footpring_out_log_file(global_memory_footprint_out_log_file_name, std::ios::out | std::ios::binary);

    if (!global_memory_footpring_out_log_file.is_open()) {
        std::cout << "Could not open file: " << global_memory_footprint_out_log_file_name << " for writing global_memory log file" << '\n';
        exit(-1);
    }


    for (auto const& result_entry : global_frame_memory_footprint_per_iso_value) {
        global_memory_footpring_out_log_file << result_entry.first << "\t" << result_entry.second.second << '\n';
    }

    global_memory_footpring_out_log_file.close();
    std::cout << "Wrote global memory footprint log file: " << global_memory_footprint_out_log_file_name << '\n';
   // exit(-1);
}
void IsosurfaceRendererApp::write_per_component_memory_footprint_stats_log_file_() {
    std::string const per_memory_component_timing_log_file_name = compile_log_file_name_(".mem_fp_log", "per_component");


    std::ofstream per_memory_footprint_out_log_file(per_memory_component_timing_log_file_name, std::ios::out | std::ios::binary);

    if (!per_memory_footprint_out_log_file.is_open()) {
        std::cout << "Could not open file: " << per_memory_component_timing_log_file_name << " for writing per memory component footprint logs" << '\n';
        exit(-1);
    }

    for (auto const& result_entry : per_component_memory_footprint_per_iso_value) {
        per_memory_footprint_out_log_file << result_entry.first << '\t'
            << result_entry.second.second.num_byte_for_vertex_indices << '\t'
            << result_entry.second.second.num_byte_for_vertex_position << '\t'
            << result_entry.second.second.num_byte_for_vertex_normal << '\t'

            << result_entry.second.second.num_byte_for_block_draw_descriptor_persistent_part << '\t'
            << result_entry.second.second.num_byte_for_block_draw_descriptor_live_part << '\t'
            << result_entry.second.second.num_byte_for_meshlet_draw_descriptor << '\t'
            << result_entry.second.second.num_byte_for_culling_info << '\t'

            << result_entry.second.second.total_num_byte;


    }
    
    per_memory_footprint_out_log_file.close();

    std::cout << "Wrote per memory footprint component log file: " << per_memory_component_timing_log_file_name << '\n';
   // exit(-1);
}

void IsosurfaceRendererApp::start_render_loop(initial_rendering_mode_settings_t const& initial_render_mode_settings) {
    renderer_->set_max_num_occupied_blocks_for_region_analysis_kernels(app_descriptor_.eval_settings.num_PMB_cells_allowed_per_mesh_shading_workgroup);

    if (initial_render_mode_settings.perform_reference_meshlet_construction) {
        renderer_->set_active_indirect_rendering_compute_mode(framework::IndirectRenderingComputeMode::MESHOPT_OFFLINE_MESHLET_COMPUTATION);
    }
    else {
        if (initial_render_mode_settings.use_task_mesh_shader_pipeline_for_persistent_extraction) {
            renderer_->set_active_indirect_rendering_compute_mode(framework::IndirectRenderingComputeMode::TASK_MESH_SHADER_BASED_EXTRACTION);
        }
        else {
            renderer_->set_active_indirect_rendering_compute_mode(framework::IndirectRenderingComputeMode::COMPUTE_SHADER_BASED_EXTRACTION);
        }
    }
    

    renderer_->set_isovalue(app_descriptor_.start_iso_value);
    renderer_->set_shading_base_color({ app_descriptor_.shading_base_color[0],app_descriptor_.shading_base_color[1], app_descriptor_.shading_base_color[2] });

    renderer_->set_active_model_yaw_pitch_roll_angles_in_degree(app_descriptor_.initial_model_transform_settings.yaw, app_descriptor_.initial_model_transform_settings.pitch, app_descriptor_.initial_model_transform_settings.roll);

    renderer_->set_reextract_always(app_descriptor_.eval_settings.always_reextract);

    if (app_descriptor_.eval_settings.extract_bounding_volumes) {
        renderer_->renderer_conf.isosurface.meshlet_forward_rendering_pipeline_mode = framework::MeshletForwardRenderPipelineMode::TASK_AND_MESH_SHADER_ONLINE_CULLING;
    }
    else {
        renderer_->renderer_conf.isosurface.meshlet_forward_rendering_pipeline_mode = framework::MeshletForwardRenderPipelineMode::MESH_SHADER_ONLY_NO_CULLING;
    }
    renderer_->renderer_conf.isosurface.persistent_meshlet_visualization_mode = framework::MeshletVisualizationMode::PHONG_SHADED;

    //configure 
    if (app_descriptor_.is_in_evaluation_mode) {
        renderer_->renderer_conf.general.render_auxiliaries = false;

        if (app_descriptor_.eval_settings.evaluate_individual_pipeline_timing_components) {
            renderer_->set_gpu_timer_mode(framework::timer_mode::PER_PIPELINE_COMPONENT); //measure all pipeline components individually
        }
        else {
            renderer_->set_gpu_timer_mode(framework::timer_mode::GLOBAL_FRAME); //measure all pipeline components individually
        }
    }

    float step_direction = 0.0f;

    if (app_descriptor_.eval_settings.perform_iso_range_evaluation) {
        if (std::numeric_limits<float>::max() == app_descriptor_.eval_settings.end_iso_value ||
            app_descriptor_.eval_settings.end_iso_value == app_descriptor_.start_iso_value) {
            std::cerr << "Can not perform range evaluation since no valid iso value range (--start_iso_value & --end_iso_value) was provided." << '\n';
            std::cout << "Exiting." << '\n';
            std::cout << '\n';
            std::exit(-1);
        }
        else {
            step_direction = std::signbit(app_descriptor_.eval_settings.end_iso_value - app_descriptor_.start_iso_value) ? -1.0f : 1.0f;
        }

    }


    constexpr uint32_t num_frames_to_warm_up = 1000;
    uint32_t num_frames_already_warmed_up = 0;


    const uint32_t num_samples_to_take = app_descriptor_.eval_settings.evaluation_samples_per_measurement;

    std::cout << "NUM SAMPLES TO TAKE: " << num_samples_to_take << '\n';
    uint32_t num_samples_already_taken = 0;

    std::vector<double> sample_buffer(num_samples_to_take, 0.0);
    std::vector<bool> outlier_flag_array(num_samples_to_take, false);

    double accumulated_values = 0.0;

    double current_iso_value = app_descriptor_.start_iso_value;
    
    bool is_first_frame = true;
    while (renderer_->is_running() ) {
        if (is_first_frame || 0 != memcmp(glm::value_ptr(app_descriptor_.background_color), glm::value_ptr(app_descriptor_.last_frames_background_color), sizeof(glm::vec3))) {
            renderer_->set_clear_color({ app_descriptor_.background_color[0], app_descriptor_.background_color[1], app_descriptor_.background_color[2], 1.0 });
            
            app_descriptor_.last_frames_background_color = app_descriptor_.background_color;
        }

        renderer_->operate_pre_frame();
        renderer_->render_frame();
        renderer_->operate_post_frame();

        if (!app_descriptor_.is_in_evaluation_mode) {
            trigger_imgui_frame_();
        }
        else {



            if (num_frames_already_warmed_up < num_frames_to_warm_up) {
                ++num_frames_already_warmed_up;
            }
            else {
                float current_iso_value = renderer_->get_isovalue();


                auto const& per_frame_performance = renderer_->get_per_frame_statistics();

                ++num_samples_already_taken;

                //FRAME TIME
                {
                    framework::timer_mode const current_gpu_frame_timer_mode = renderer_->get_gpu_timer_mode();
                    switch (current_gpu_frame_timer_mode) {
                    case framework::timer_mode::GLOBAL_FRAME: {

                        auto& curr_pair = global_frame_times_per_iso_value[current_iso_value];
                        curr_pair.first += 1;
                        curr_pair.second += per_frame_performance.measured_time.global_time_ms;

                        if (num_samples_to_take == num_samples_already_taken) {
                            curr_pair.second /= curr_pair.first;
                        }
                        break;
                    } //end of global frame timer logging branch

                    case framework::timer_mode::PER_PIPELINE_COMPONENT: {

                        auto& curr_pair = per_component_frame_times_per_iso_value[current_iso_value];
                        curr_pair.first += 1;

                        auto& relevant_components = curr_pair.second;
                        relevant_components.occupancy_filtering += per_frame_performance.measured_time.per_component.dense_occupancy_computation_time_ms;
                        relevant_components.extraction += per_frame_performance.measured_time.per_component.pure_extraction_time_ms;
                        relevant_components.rendering += per_frame_performance.measured_time.per_component.pure_isosurface_rendering_time_ms;


                        if (num_samples_to_take == num_samples_already_taken) {
                            relevant_components.occupancy_filtering /= curr_pair.first;
                            relevant_components.extraction /= curr_pair.first;
                            relevant_components.rendering /= curr_pair.first;
                        }
                        break;
                    } //end of per cp frame timer logging branch
                    }
                }


                
                //FRAME PERSISTENT MEMORY FOOTPRINT
                {
                    framework::memory_footprint_mode const current_memory_footprint_mode = renderer_->get_gpu_memory_footprint_mode();
                    switch (current_memory_footprint_mode) {
                    case framework::memory_footprint_mode::GLOBAL_FRAME: {

                        auto& curr_pair =  global_frame_memory_footprint_per_iso_value[current_iso_value];
                        curr_pair.first += 1;
                        curr_pair.second += per_frame_performance.measured_memory_footprints.total_amount_byte;


                       // ++num_samples_already_taken;


                        if (num_samples_to_take == num_samples_already_taken) {


                            curr_pair.second /= curr_pair.first;
                        }
                        break;
                    } //end of global frame timer logging branch

                    case framework::memory_footprint_mode::PER_MEMORY_COMPONENT: {

                        auto& curr_pair = per_component_memory_footprint_per_iso_value[current_iso_value];
                        curr_pair.first += 1;

                        auto& relevant_components = curr_pair.second;
                        relevant_components.num_byte_for_vertex_position += per_frame_performance.measured_memory_footprints.separated_memory_amounts_byte.vertex_positions;
                        relevant_components.num_byte_for_vertex_normal += per_frame_performance.measured_memory_footprints.separated_memory_amounts_byte.vertex_normals;
                        relevant_components.num_byte_for_vertex_indices += per_frame_performance.measured_memory_footprints.separated_memory_amounts_byte.indices;
                        
                        relevant_components.num_byte_for_block_draw_descriptor_persistent_part += per_frame_performance.measured_memory_footprints.separated_memory_amounts_byte.block_descriptors_persistent_part;
                        relevant_components.num_byte_for_block_draw_descriptor_live_part += per_frame_performance.measured_memory_footprints.separated_memory_amounts_byte.block_descriptors_live_part;
                        relevant_components.num_byte_for_meshlet_draw_descriptor += per_frame_performance.measured_memory_footprints.separated_memory_amounts_byte.meshlet_descriptors;
                        relevant_components.num_byte_for_culling_info += per_frame_performance.measured_memory_footprints.separated_memory_amounts_byte.culling_information;

                        relevant_components.total_num_byte += per_frame_performance.measured_memory_footprints.separated_memory_amounts_byte.total;


                        if (num_samples_to_take == num_samples_already_taken) {
                            relevant_components.num_byte_for_vertex_position /= curr_pair.first;
                            relevant_components.num_byte_for_vertex_normal /= curr_pair.first;
                            relevant_components.num_byte_for_vertex_indices /= curr_pair.first;

                            relevant_components.num_byte_for_block_draw_descriptor_persistent_part /= curr_pair.first;
                            relevant_components.num_byte_for_block_draw_descriptor_live_part /= curr_pair.first;
                            relevant_components.num_byte_for_meshlet_draw_descriptor /= curr_pair.first;
                            relevant_components.num_byte_for_culling_info /= curr_pair.first;

                            relevant_components.total_num_byte /= curr_pair.first;
                        }
                        break;
                    } //end of per cp frame timer logging branch
                    }
                }

                //*/
                //evaluate_individual_pipeline_timing_components

                if (num_samples_to_take == num_samples_already_taken) {
                    bool done_measuring = false;
                    if (app_descriptor_.eval_settings.perform_iso_range_evaluation) {
                        float incement = app_descriptor_.eval_settings.iso_range_evalation_step_size * step_direction;
                        float potential_next_iso_value = current_iso_value + incement;
                        if (potential_next_iso_value <= app_descriptor_.eval_settings.end_iso_value) {
                            current_iso_value = potential_next_iso_value;
                            num_samples_already_taken = 0;
                            num_frames_already_warmed_up = 0;
                            renderer_->set_isovalue(current_iso_value);
                        }
                        else {
                            done_measuring = true;
                        }
                    }
                    else {
                        done_measuring = true;
                    }




                    if (done_measuring) {
                        if (app_descriptor_.eval_settings.evaluate_individual_pipeline_timing_components) {
                            write_per_component_frame_time_log_file_();
                        }
                        else {
                            write_global_frame_time_log_file_();
                        }

                        if (app_descriptor_.eval_settings.log_memory_footprint_after_extraction) {
                            if (app_descriptor_.eval_settings.evaluate_individual_pipeline_memory_components) {
                                write_per_component_memory_footprint_stats_log_file_();
                            }
                            else {
                                write_global_memory_footprint_stats_log_file_();
                            }
                        }

                        exit(-1);
                    }
                }

            }
        }


        renderer_->finalize_frame();
        is_first_frame = false;
    }
}

void IsosurfaceRendererApp::initialize_framework(AppRunDescriptor const& run_descriptor) {
    renderer_ = std::make_unique<framework::OpenGLRenderer>(app_descriptor_.geom_buffer_settings.index_buffer_size_in_MB, app_descriptor_.geom_buffer_settings.position_buffer_size_in_MB, app_descriptor_.geom_buffer_settings.normal_buffer_size_in_MB);

    if (run_descriptor.eval_settings.log_memory_footprint_after_extraction) {

        if (run_descriptor.eval_settings.evaluate_individual_pipeline_memory_components) {
            renderer_->set_gpu_memory_footprint_mode(framework::memory_footprint_mode::PER_MEMORY_COMPONENT);
        }
        else {
            renderer_->set_gpu_memory_footprint_mode(framework::memory_footprint_mode::GLOBAL_FRAME);
        }
    }

    if (run_descriptor.precompile_shaders) {
        renderer_->precompile_shaders();
    }

    renderer_->initialize(run_descriptor.window_resolution);
    renderer_->register_window_callbacks();    
}

void IsosurfaceRendererApp::tear_down_framework() {
    renderer_->tear_down();
}

void IsosurfaceRendererApp::load_volumes(std::string const& volume_resource_string,
                                          glm::ivec3 const& persistent_extraction_block_size) {
    renderer_->register_volume_dataset(volume_resource_string, persistent_extraction_block_size);
}

void IsosurfaceRendererApp::create_shader_programs() {
    if (app_descriptor_.disable_shader_cache) {
        framework::shader_program::set_binary_cache_enabled(false);
    }
    renderer_->create_shader_programs();
}

void IsosurfaceRendererApp::trigger_imgui_frame_() {
    if (!renderer_) {
        return;
    }
    
    if (draw_gui_) {
        renderer_->start_gui_frame();
        renderer_->end_gui_frame();
    }
}
