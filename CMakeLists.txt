cmake_minimum_required(VERSION 3.22...3.27 FATAL_ERROR)

project(isosurface_rendering_framework LANGUAGES C CXX)


configure_file(${CMAKE_SOURCE_DIR}/cmake_config_in/framework_cmake_config.hpp.in
               ${CMAKE_SOURCE_DIR}/config/framework_cmake_config.hpp @ONLY)


#set the build type if its not set
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

if (UNIX)
  find_package(PkgConfig)
endif(UNIX)

set(CMAKE_CXX_STANDARD 20)

# Location where cmake first looks for modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(GLOBAL_EXT_DIR ${CMAKE_SOURCE_DIR}/external)

#find_package (Threads REQUIRED)


# Add output directory
if(MSVC)
  set(BINARY_DIRECTORY build)
endif()
# MSVC & Xcode automatically create the build-type folders
if(MSVC OR CMAKE_GENERATOR STREQUAL Xcode)
  SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
  SET(LIBRARY_OUTPUT_PATH  ${PROJECT_BINARY_DIR})
else()
  SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
  SET(LIBRARY_OUTPUT_PATH  ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()

set(CMAKE_INSTALL_RPATH "$ORIGIN")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# gl3w is used for loading GL extensions such as the NV task-mesh-shader ones
set(GL3W_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/include)
set(GL3W_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/src/GL/gl3w.c)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
set(GLFW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)

# meshoptimizer by Arseny Kapoulkine (https://github.com/zeux/meshoptimizer)
# is used for creating a basis for a highly optimized reference mesh
add_subdirectory(external/meshoptimizer EXCLUDE_FROM_ALL)

################################
# activate C++ 20
if(MSVC)
    # build in parallel, show warnings
    message([STATUS] "building for MSCV")
    #add_definitions(/MP /W3 /openmp)
else()
    # show warnings
    #add_definitions(-Wall -Wextra)
    # force linking  with c++20 lib
endif()

################################
# clang-tidy
# Enable with: -DENABLE_CLANG_TIDY=ON
# Set after the external add_subdirectory() calls above so external targets
# (glfw, meshoptimizer) are NOT linted. Targets created below (framework,
# isosurface_renderer_app) inherit CMAKE_CXX_CLANG_TIDY at creation time.
################################
option(ENABLE_CLANG_TIDY "Run clang-tidy during build on framework/ and apps/" OFF)
if(ENABLE_CLANG_TIDY)
    # On Windows, Visual Studio 2022 bundles clang-tidy under VC/Tools/Llvm
    # but does not add it to PATH, so include those locations as hints.
    set(_CLANG_TIDY_HINTS "")
    if(WIN32)
        foreach(_vs_edition Enterprise Professional Community BuildTools)
            list(APPEND _CLANG_TIDY_HINTS
                "C:/Program Files/Microsoft Visual Studio/2022/${_vs_edition}/VC/Tools/Llvm/x64/bin"
                "C:/Program Files/Microsoft Visual Studio/2022/${_vs_edition}/VC/Tools/Llvm/bin"
                "C:/Program Files (x86)/Microsoft Visual Studio/2022/${_vs_edition}/VC/Tools/Llvm/x64/bin"
                "C:/Program Files (x86)/Microsoft Visual Studio/2022/${_vs_edition}/VC/Tools/Llvm/bin")
        endforeach()
        list(APPEND _CLANG_TIDY_HINTS
            "C:/Program Files/LLVM/bin"
            "C:/Program Files (x86)/LLVM/bin")
    endif()

    find_program(CLANG_TIDY_EXE
                 NAMES clang-tidy clang-tidy.exe
                 HINTS ${_CLANG_TIDY_HINTS}
                 DOC "Path to clang-tidy executable")
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
        # --extra-arg silences warnings about MSVC-only flags that clang doesn't know.
        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_EXE};--extra-arg-before=-Wno-unknown-warning-option")
    else()
        message(WARNING "ENABLE_CLANG_TIDY=ON but clang-tidy was not found. "
                        "Pass -DCLANG_TIDY_EXE=<path> to specify it explicitly. Skipping.")
    endif()
endif()

# the folder 'framework' contains the framework code for loading and using the different visualization modes
add_subdirectory(framework)

# actual app directory containing the code for compiling 'direct_volume_renderer_app'
add_subdirectory(apps)

################################
#if user didnt set install dir, override it and write it to the cache -> Type and description necessary,
#overwrite variable, not just write it to cache
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE STRING "Install path prefix, prepended onto install directories." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

