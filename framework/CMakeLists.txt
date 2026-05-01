set(BASE_IMPL_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)

set(OPENGL_IMPL_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/OpenGL_impl/)

# main section for interface file grouping


set(FRAMEWORK_SHADER_DIR_GLSL ../glsl/)

set(FRAMEWORK_PLATFORM_FILE
    ${BASE_IMPL_DIRECTORY}/framework_platform.hpp
) 

set(FRAMEWORK_HEADER_FILES
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/atomic_counter_buffer_config.h.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/per_frame_indirect_draw_parameter_ssbo_config.h.glsl
    ${BASE_IMPL_DIRECTORY}/available_shader_programs.hpp

    ${BASE_IMPL_DIRECTORY}/CPU_UBO_types.hpp

    ${OPENGL_IMPL_DIRECTORY}/opengl_UBOs.hpp
    
    ${BASE_IMPL_DIRECTORY}/shader_program.hpp

    ${BASE_IMPL_DIRECTORY}/base_gpu_timer.hpp
    ${OPENGL_IMPL_DIRECTORY}/opengl_gpu_timer.hpp
)

set(RENDERER_HEADER_FILES
    ${BASE_IMPL_DIRECTORY}/base_renderer.hpp
    ${BASE_IMPL_DIRECTORY}/base_renderer_types.hpp

    ${OPENGL_IMPL_DIRECTORY}/opengl_renderer.hpp
)


set(GPU_CONTEXT_HEADER_FILES 
    ${BASE_IMPL_DIRECTORY}/base_graphics_context.hpp
    ${OPENGL_IMPL_DIRECTORY}/opengl_context.hpp
    ${OPENGL_IMPL_DIRECTORY}/opengl_extensions_struct.hpp
)

set(VOLUME_HEADER_FILES 
    ${BASE_IMPL_DIRECTORY}/base_volume.hpp
    ${BASE_IMPL_DIRECTORY}/base_volume_types.hpp

    ${OPENGL_IMPL_DIRECTORY}/opengl_volume.hpp
)

set(CAMERA_HEADER_FILES
    ${BASE_IMPL_DIRECTORY}/base_camera.hpp
)


source_group(
              "Header Files/GPU_Contexts" 
              FILES 
              ${GPU_CONTEXT_HEADER_FILES}
            )

source_group(
              "Header Files/Volume" 
              FILES 
              ${VOLUME_HEADER_FILES}
            )

source_group(
              "Header Files/Renderer" 
              FILES 
              ${RENDERER_HEADER_FILES}
            )

source_group(
              "Header Files/Camera" 
              FILES 
              ${CAMERA_HEADER_FILES}
            )



# main section for implementation file grouping

set(FRAMEWORK_SOURCE_FILES
    ${BASE_IMPL_DIRECTORY}/shader_program.cpp

    ${BASE_IMPL_DIRECTORY}/base_gpu_timer.cpp
    ${OPENGL_IMPL_DIRECTORY}/opengl_gpu_timer.cpp
)

set(GPU_CONTEXT_SOURCE_FILES 
    ${BASE_IMPL_DIRECTORY}/base_graphics_context.cpp
    ${OPENGL_IMPL_DIRECTORY}/opengl_context.cpp
    )

set(VOLUME_SOURCE_FILES
    ${BASE_IMPL_DIRECTORY}/base_volume.cpp
    ${OPENGL_IMPL_DIRECTORY}/opengl_volume.cpp
    ${OPENGL_IMPL_DIRECTORY}/opengl_volume_gpu_upload.cpp
    )

set(RENDERER_SOURCE_FILES
    ${BASE_IMPL_DIRECTORY}/base_renderer.cpp
    ${BASE_IMPL_DIRECTORY}/base_renderer_gui_definition.cpp
    ${BASE_IMPL_DIRECTORY}/base_renderer_key_callback.cpp
    ${BASE_IMPL_DIRECTORY}/base_renderer_create_shader_programs.cpp

    ${OPENGL_IMPL_DIRECTORY}/opengl_renderer.cpp
    ${OPENGL_IMPL_DIRECTORY}/opengl_renderer__render_isosurface_indirectly_persistent_extraction.cpp
)

set(CAMERA_SOURCE_FILES
    ${BASE_IMPL_DIRECTORY}/base_camera.cpp
   )


source_group(
              "Source Files/GPU_Contexts" 
              FILES 
              ${GPU_CONTEXT_SOURCE_FILES}
            )

source_group(
               "Source Files/Volume"
               FILES
               ${VOLUME_SOURCE_FILES}
             )

source_group(
               "Source Files/Renderer"
               FILES
               ${RENDERER_SOURCE_FILES}
)

source_group(
               "Source Files/Camera"
               FILES
               ${CAMERA_SOURCE_FILES}
)

set(IMGUI_SOURCES 
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui.h
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp
	${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp
	${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp
	${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp
	${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
	${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp

	${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    )

set(IMPLOT_SOURCES
    ${CMAKE_SOURCE_DIR}/external/implot/implot.h
	${CMAKE_SOURCE_DIR}/external/implot/implot.cpp
	${CMAKE_SOURCE_DIR}/external/implot/implot_internal.h
    ${CMAKE_SOURCE_DIR}/external/implot/implot_items.cpp
)

set(FRAMEWORK_SOURCE 
    ${FRAMEWORK_PLATFORM_FILE}

    ${GPU_CONTEXT_HEADER_FILES}
    ${GPU_CONTEXT_SOURCE_FILES}
    
    ${FRAMEWORK_HEADER_FILES}
    ${FRAMEWORK_SOURCE_FILES}

    ${VOLUME_HEADER_FILES}
    ${VOLUME_SOURCE_FILES}

    ${RENDERER_HEADER_FILES}
    ${RENDERER_SOURCE_FILES}

    ${CAMERA_HEADER_FILES}
    ${CAMERA_SOURCE_FILES}

    ${CMAKE_CURRENT_SOURCE_DIR}/framework_utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/gl3w/src/GL/gl3w.c



    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
)

 
# main section for shader file file grouping



set(GLSL_SHADER_INCLUDE_FILES
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/abstract_task_shader_submission_interface.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/abstract_mesh_shader_submission_interface.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/atomic_counter_buffer_config.h.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/atomic_counters.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/const_memory_geometry.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/culling.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/defines.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/marching_cubes_tables.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/math.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/per_frame_indirect_draw_parameter_ssbo_config.h.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/per_frame_indirect_draw_parameter_ssbo.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/shading.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/ssbos.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/ubos.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/shader_includes/volumes.glsl

)

source_group("glsl/shader_includes" FILES ${GLSL_SHADER_INCLUDE_FILES} )

set(GLSL_INDIRECT_PERSISTENT_EXTRACTION_SHADER_FILES
    ${FRAMEWORK_SHADER_DIR_GLSL}/task_mesh_shader_based_isosurface_extraction/extraction_task_mesh_shader_interface.glsl
    ${FRAMEWORK_SHADER_DIR_GLSL}/task_mesh_shader_based_isosurface_extraction/region_size_analysis.task
    ${FRAMEWORK_SHADER_DIR_GLSL}/task_mesh_shader_based_isosurface_extraction/isosurface_extraction.mesh
)

source_group("glsl/task_mesh_shader_based_extraction" FILES ${GLSL_INDIRECT_PERSISTENT_EXTRACTION_SHADER_FILES} )

set(GLSL_PERSISTENT_ISOSURFACE_EXTRACTION_SHADER_FILES
    ${FRAMEWORK_SHADER_DIR_GLSL}/compute_shader_based_isosurface_extraction/min_max_computation_blockwise.comp
    ${FRAMEWORK_SHADER_DIR_GLSL}/compute_shader_based_isosurface_extraction/dense_occupancy_buffer_computation.comp
    ${FRAMEWORK_SHADER_DIR_GLSL}/compute_shader_based_isosurface_extraction/parallel_marching_blocks.comp
    ${FRAMEWORK_SHADER_DIR_GLSL}/compute_shader_based_isosurface_extraction/snapdiv_group_occupied_blocks.comp
)

source_group("glsl/persistent_isosurface_extraction" FILES ${GLSL_PERSISTENT_ISOSURFACE_EXTRACTION_SHADER_FILES} )

set(GLSL_FORWARD_RENDERING_TRIANGLE_MODE_SHADER_FILES
    ${FRAMEWORK_SHADER_DIR_GLSL}/forward_rendering/triangle_mode/triangle_forward_rendering_from_VBO_phong.vert

    #task mesh shader interface
    ${FRAMEWORK_SHADER_DIR_GLSL}/forward_rendering/triangle_mode/triangle_forward_rendering_task_mesh_shader_interface.glsl

    # task + mesh shader
    ${FRAMEWORK_SHADER_DIR_GLSL}/forward_rendering/triangle_mode/triangle_forward_rendering_from_meshlet_buffers_general_task_and_mesh.task
    #mesh only
    ${FRAMEWORK_SHADER_DIR_GLSL}/forward_rendering/triangle_mode/triangle_forward_rendering_from_meshlet_buffers_general_geometry_setup.mesh
)

source_group("glsl/forward_rendering/triangle_mode" FILES ${GLSL_FORWARD_RENDERING_TRIANGLE_MODE_SHADER_FILES} )


set(GLSL_AUXILIARY_VISUALIZATIONS_SHADER_FILES
    ${FRAMEWORK_SHADER_DIR_GLSL}/auxiliary_visualizations/culling_reference_camera_frustum_line_vis.mesh
    ${FRAMEWORK_SHADER_DIR_GLSL}/auxiliary_visualizations/culling_reference_camera_frustum_line_vis.frag

    ${FRAMEWORK_SHADER_DIR_GLSL}/auxiliary_visualizations/coordinate_system_axes_vis.vert
    ${FRAMEWORK_SHADER_DIR_GLSL}/auxiliary_visualizations/coordinate_system_axes_vis.frag

    ${FRAMEWORK_SHADER_DIR_GLSL}/auxiliary_visualizations/volume_boundary_vis.mesh
    ${FRAMEWORK_SHADER_DIR_GLSL}/auxiliary_visualizations/volume_boundary_vis.frag
)

source_group("glsl/auxiliary_visualizations" FILES ${GLSL_AUXILIARY_VISUALIZATIONS_SHADER_FILES} )

set(GLSL_COMMON_SHADING_MODELS_SHADER_FILES
    ${FRAMEWORK_SHADER_DIR_GLSL}/common_shading_models/common_VBO_and_meshlet_buffers_phong.frag
)

source_group("glsl/common_shading_models" FILES ${GLSL_COMMON_SHADING_MODELS_SHADER_FILES} )


set(EXTERNAL_ISOSURFACE_EXTRACTION_FILES ${CMAKE_SOURCE_DIR}/external/mc_tables_nvidia/tables.h)
set_source_files_properties(${EXTERNAL_ISOSURFACE_EXTRACTION_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)




set(FRAMEWORK_SHADER_FILES
    ${GLSL_SHADER_INCLUDE_FILES}
    ${GLSL_PERSISTENT_ISOSURFACE_EXTRACTION_SHADER_FILES}
    ${GLSL_INDIRECT_PERSISTENT_EXTRACTION_SHADER_FILES}
    ${GLSL_FORWARD_RENDERING_TRIANGLE_MODE_SHADER_FILES}
    ${GLSL_AUXILIARY_VISUALIZATIONS_SHADER_FILES}
    ${GLSL_COMMON_SHADING_MODELS_SHADER_FILES}
)


source_group("Inline Files" FILES ${FRAMEWORK_INLINE})


add_library(framework SHARED 
            ${FRAMEWORK_SOURCE} 
            ${FRAMEWORK_INLINE} 
            ${FRAMEWORK_HEADER} 
            ${FRAMEWORK_SHADER_FILES}
            ${GL3W_SOURCE}

            ${EXTERNAL_ISOSURFACE_EXTRACTION_FILES}
)

# link against GL-libraries and meshoptimizer ()
if(MSVC)
  target_link_libraries(framework OpenGL32 ${CMAKE_THREAD_LIBS_INIT} meshoptimizer)
  target_compile_definitions(framework PUBLIC NOMINMAX)
  target_compile_options(framework PRIVATE "/MP")
else()
  # linking omp here breaks the gh actions workflow
  target_link_libraries(framework GL ${CMAKE_THREAD_LIBS_INIT} meshoptimizer)
  set_target_properties(meshoptimizer PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# sets OpenMP linker flags automatically such that OMP is actually properly enabled by default (like -fopenmp under linux or /openmp on windows)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

target_include_directories(framework PUBLIC ${OpenMP_C_INCLUDE_DIRS})
target_link_libraries(framework glfw ${OpenMP_C_LIBRARIES})




target_include_directories(framework SYSTEM PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(framework SYSTEM PUBLIC ${GL3W_INCLUDE_DIR})


target_include_directories(framework SYSTEM PUBLIC ${GLFW_INCLUDE_DIR})


################################
# glm
################################
set(GLM_DIRECTORY glm)
set(GLM_INCLUDE_DIR /external/${GLM_DIRECTORY})
target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/${GLM_DIRECTORY})
target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/shaderc/libshaderc/include)
################################


# bs::threadpool by bshoshany (https://github.com/bshoshany/thread-pool)
# is used for submitting work of dependent kernel via jobs in an asynchronous manner
target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/thread-pool-3.5.0/include)

target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/meshoptimizer/src)
target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/)

target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/imgui/)
target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/implot/)
target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/imgui_club/imgui_memory_editor/)
target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/external/ImViewGuizmo/)



target_include_directories(framework PRIVATE ${CMAKE_SOURCE_DIR}/config/)


################################
# CLI11
################################
set(CLI11_DIRECTORY CLI11)
set(CLI11_INCLUDE_DIR /external/${CLI11_DIRECTORY})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/external/${CLI11_DIRECTORY})
################################



#install to whatever project is the CMAKE source dir ...
#if(MSVC)
   
	install(TARGETS framework DESTINATION bin)
   
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../glsl
	  DESTINATION bin
          FILES_MATCHING
          PATTERN "*.vert"
          PATTERN "*.geom"
          PATTERN "*.tess_cont"
          PATTERN "*.tess_eval"
          PATTERN "*.frag"
          PATTERN "*.comp"
          PATTERN "*.task"
          PATTERN "*.mesh"
          PATTERN "*.glsl"
	)

    install(DIRECTORY DESTINATION bin)



set(framework_INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR})
