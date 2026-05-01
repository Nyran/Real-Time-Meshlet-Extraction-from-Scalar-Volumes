#define target for direct volume renderer app

set(ARCBALL_IMPL_SOURCES
    ${CMAKE_SOURCE_DIR}/external/arcball_implementation/arcball.hpp
    ${CMAKE_SOURCE_DIR}/external/arcball_implementation/matrix.hpp
    ${CMAKE_SOURCE_DIR}/external/arcball_implementation/quaternion.hpp
    ${CMAKE_SOURCE_DIR}/external/arcball_implementation/vectors.hpp
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

add_executable(isosurface_renderer_app 
               main.cpp 
               isosurface_renderer_app.cpp isosurface_renderer_app.hpp
			   ../../config/framework_cmake_config.hpp

			   ${PYTHON_EVALUATION_SCRIPTS}
			   ${PYTHON_PLOTTING_SCRIPTS}
			  )

target_include_directories(isosurface_renderer_app PRIVATE ${CMAKE_SOURCE_DIR}/external/)
target_include_directories(isosurface_renderer_app PRIVATE ${CMAKE_SOURCE_DIR}/external/arcball_implementation/)
target_include_directories(isosurface_renderer_app PRIVATE ${CMAKE_SOURCE_DIR}/config) 


target_include_directories(isosurface_renderer_app PRIVATE ${CMAKE_SOURCE_DIR}/external/shaderc/libshaderc/include)

################################
# glm
################################
set(GLM_DIRECTORY glm)
set(GLM_INCLUDE_DIR /external/${GLM_DIRECTORY})
target_include_directories(isosurface_renderer_app PRIVATE ${CMAKE_SOURCE_DIR}/external/${GLM_DIRECTORY})
################################

################################
# CLI11
################################
set(CLI11_DIRECTORY CLI11)
set(CLI11_INCLUDE_DIR /external/${CLI11_DIRECTORY})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/external/${CLI11_DIRECTORY})
################################


if(MSVC) #windows path
    target_link_libraries(isosurface_renderer_app OpenGL32 glfw framework)
else(MSVC) #ubuntu path
	target_link_libraries(isosurface_renderer_app GL glfw framework)
endif(MSVC)


# sets OpenMP linker flags automatically such that OMP is actually properly enabled by default (like -fopenmp under linux or /openmp on windows)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

target_include_directories(isosurface_renderer_app PUBLIC ${OpenMP_C_INCLUDE_DIRS})
target_link_libraries(isosurface_renderer_app ${OpenMP_C_LIBRARIES})



install(TARGETS isosurface_renderer_app 
		RUNTIME_DEPENDENCIES 
		PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
		POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" DESTINATION bin)
