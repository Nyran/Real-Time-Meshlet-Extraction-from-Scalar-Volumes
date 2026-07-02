![Teaser of our Real-Time Meshlet Extraction Approach comparing against a state-of-the-art real-time indexed triangle extraction approach and offline created meshlets.](https://github.com/Nyran/Real-Time-Meshlet-Extraction-from-Scalar-Volumes/paper_teaser/teaser.jpg)

# Supplementary Core Framework Code for EGPGV 2026 paper: Real-Time Meshlet Extraction from Scalar Volumes

***

## Public GitHub-Repository
A maintainable version of this repository is available at the following public GitHub repository:: https://github.com/Nyran/Real-Time-Meshlet-Extraction-from-Scalar-Volumes

## Hardware and Software Requirements
In order to successfully execute the framework, your machine should be equipped with the following:

### Hardware
* Discrete NVIDIA GPU of Turing generation or later (e.g. RTX 20X0, RTX 30X0, RTX 40X0, ..., Quadro RTX). The framework relies on `NV_mesh_shader` and other NVIDIA-specific GL extensions.

### Software
* Recent NVIDIA drivers (e.g. NVIDIA driver 572.60). Valid GLSL syntax for the NVIDIA-specific extensions can differ between drastically different driver versions.
* CMake **>= 3.22**.
* A C++20 compiler. We use MSVC from Visual Studio 2022 on Windows and GCC/Clang on Linux.
* OpenMP (Linux: install your distribution's `libomp`/`libgomp` development package).

**We tested the framework with an NVIDIA RTX 4090 GPU and driver version 572.60 under both Linux and Windows, so this combination should work for you as well. Slightly differing driver versions are expected to work as well.**

***

# Project Structure
Besides external libraries, volume data, CMake files and auxiliary scripts, the project can be coarsely divided into three parts:
* Application code in the folder `apps/`
* Framework code in the folder `framework/`
* Shader code in the folder `glsl/`

Those three main parts are explained briefly below.

## Application Code — folder `apps/`
The single application target is `isosurface_renderer_app`, defined in `apps/isosurface_renderer/`. It is mainly relevant for understanding the command-line parameters exposed by the core framework (initial isovalue, default rotation, evaluation flags, etc.). The CLI is parsed in `apps/isosurface_renderer/main.cpp`.

## Framework Code — folder `framework/`
The framework code handles shader-pipeline creation, CPU↔GPU interaction, volume loading, and so on.

For understanding how the task/mesh-shader pipelines fit together, the most important file is `framework/base_renderer_create_shader_programs.cpp`. There you can see which shaders form a pipeline and are therefore executed together as a combination of task, mesh, and fragment shaders. These shaders are split across subfolders of `glsl/` according to the rendering mode they belong to.

## Shader Code — folder `glsl/`
The `glsl/` folder contains the human-readable shader code which is the core of our rendering method and the reference methods. The shader code is separated into subfolders by rendering mode:

* `glsl/task_mesh_shader_based_isosurface_extraction/` — task+mesh-shader persistent extraction pipeline
* `glsl/compute_shader_based_isosurface_extraction/` — compute-shader persistent extraction (PMB-style)
* `glsl/forward_rendering/triangle_mode/` — forward rendering of the extracted geometry
* `glsl/auxiliary_visualizations/` — debug visualizations (frustum, bounding boxes, axes, ...)
* `glsl/common_shading_models/`, `glsl/shader_includes/` — shared shader infrastructure

***

# Pipeline Overview (Where to Start Reading)

The framework runs a two-stage GPU pipeline per frame: **(1) extract** isosurface geometry into persistent SSBOs, then **(2) forward-render** the extracted meshlets. The default mode (`--task_mesh_persistent`) implements the method described in the paper; the compute-shader fallback path (PMB) is provided for reference.

## Stage 1 — Persistent Extraction (paper Section 3.2)

The volume is divided into **8×8×4-cell extraction blocks**. Per frame, the extraction pipeline runs:

1. **Per-block min/max & dense occupancy.** A pair of compute shaders fills a single-level per-block min/max texture and writes the dense list of **occupied blocks** (blocks whose value range straddles the current isovalue):
    * `glsl/compute_shader_based_isosurface_extraction/min_max_computation_blockwise.comp`
    * `glsl/compute_shader_based_isosurface_extraction/dense_occupancy_buffer_computation.comp`
    * `glsl/compute_shader_based_isosurface_extraction/snapdiv_group_occupied_blocks.comp` (groups occupied blocks for indirect mesh-shader dispatch)

2. **Region analysis (task shader).** For each occupied 8×8×4 block, the task shader hierarchically decides whether the isosurface fits into a single meshlet at the largest subblock size (8×8×4), a medium size (4×4×4), or the smallest fallback size (2×2×2), and emits one mesh-shader workgroup per chosen subblock.
    * `glsl/task_mesh_shader_based_isosurface_extraction/region_size_analysis.task` — paper Sec. 3.2.1, supplementary Listing 1

3. **Meshlet extraction (mesh shader).** Each spawned mesh-shader workgroup extracts exactly one meshlet (vertices, normals, indices, descriptor) into the global SSBOs from its assigned subblock, using a Parallel-Marching-Blocks–style cell iteration.
    * `glsl/task_mesh_shader_based_isosurface_extraction/isosurface_extraction.mesh` — paper Sec. 3.2.2, supplementary Listing 2

The task↔mesh-shader payload contract lives in `glsl/task_mesh_shader_based_isosurface_extraction/extraction_task_mesh_shader_interface.glsl` — start there if you want to modify either side.

## Stage 2 — Meshlet Forward Rendering (paper Section 3.3)

After extraction, the renderer reads the persistent meshlet/vertex/index buffers and rasterizes them with another task+mesh-shader pair:

* `glsl/forward_rendering/triangle_mode/triangle_forward_rendering_from_meshlet_buffers_general_task_and_mesh.task` — visibility culling (paper Sec. 3.3.1)
* `glsl/forward_rendering/triangle_mode/triangle_forward_rendering_from_meshlet_buffers_general_geometry_setup.mesh` — meshlet rasterization (paper Sec. 3.3.2)
* `glsl/common_shading_models/common_VBO_and_meshlet_buffers_phong.frag` — shared Phong fragment shader

A `--reference_meshlets` mode is also available, which builds the reference meshlet set offline using [meshoptimizer](https://github.com/zeux/meshoptimizer) (Arseny Kapoulkine) and renders the same geometry through the same forward path.

## Wiring (Framework Side)

`framework/base_renderer_create_shader_programs.cpp` is the single anchor for all GPU programs in the pipeline: it lists every shader, the variant defines that specialize it, and which paper section/listing it implements. If you want to add or replace a stage, that is the file to edit.

## Block Sizing Glossary

The paper uses the following naming convention, which is reflected throughout the shader code:

* **Block** — an 8×8×4 region of cells (the largest unit a single task-shader workgroup analyses).
* **Subblock** — one of the three nested partition sizes the region analysis can pick: 2×2×2 (smallest, fallback), 4×4×4 (medium), 8×8×4 (largest = the whole block).
* **Padded** sizes (+1 voxel per dimension to read corner samples) are 3×3×3, 5×5×5, and 9×9×5.

Numeric tokens that show up in code (`_222_/_444_/_884_` for unpadded, `_333_/_555_/_995_` for padded) follow this convention; see the legend at the top of `glsl/shader_includes/defines.glsl`.

***

# Building the Project
You can build the project explicitly via the command line, or via the prepared scripts that automate the steps shown under *"Using the Command Line Interface explicitly"*.

Assume you start in the root directory (the directory in which you found this `README.md`).

## Using the Command Line Interface Explicitly
The project is provided as a CMake-Project description (https://cmake.org), which defines how the actual Visual Studio solution (Windows) or Makefiles (Unix) are generated.
```
# create an empty build directory
mkdir build
# change into the build directory
cd build

# run cmake
cmake ..
# build the project using the install target
cmake --build . --target install --config Release

# if the last command executed successfully, you should find an install
# directory in the root folder, so go back to the root folder ...
cd ..
# ... and change into 'install/bin'
cd install/bin

# in the installed bin folder you will find the built executable for the
# core code and a shader directory. Pass any of the shipped raw volumes
# via -f to start the app:
./isosurface_renderer_app -f ../../raw_volumes/head_w64_h64_d64_c1_b8.raw
```

## Using the Build Scripts
If you do not want to type all the commands by hand, you can run `build_for_linux.sh` or `build_for_windows.bat`, which contain similar build commands to the explicit way above.

## Optional: clang-tidy on the codebase
The build is wired so that clang-tidy can run alongside the compiler on every translation unit in `framework/` and `apps/` (vendored sources under `external/` are excluded via a local `.clang-tidy`). Enable it by configuring with:
```
cmake -S . -B build -DENABLE_CLANG_TIDY=ON
cmake --build build
```
On Windows the build automatically picks up the clang-tidy that ships with Visual Studio 2022 (`...\VC\Tools\Llvm\x64\bin\clang-tidy.exe`); on Linux it uses whichever `clang-tidy` is on `PATH`. If clang-tidy cannot be located you can pass `-DCLANG_TIDY_EXE=<path>` explicitly. The diagnostics surface as warnings in the Visual Studio Error List or in the compiler output. The selected check set lives in `.clang-tidy` at the repository root.

***

# Running the App (after the install step)

The app must be started from inside `install/bin/` so that the relative shader paths and the `glsl/` directory installed alongside the executable can be found.

The volume file path is **required** (`-f`/`--file`); it must encode the volume layout in its name through the tokens `_w<width>`, `_h<height>`, `_d<depth>`, `_c<channels>`, `_b<bits-per-sample>` (e.g. `head_w64_h64_d64_c1_b8.raw`).

```
# start the renderer in the default mode (compute-shader persistent extraction)
./isosurface_renderer_app -f ../../raw_volumes/head_w64_h64_d64_c1_b8.raw

# pick the initial isovalue
./isosurface_renderer_app -f ../../raw_volumes/head_w64_h64_d64_c1_b8.raw --iso_value 0.3

# use the contributed task+mesh-shader pipeline for persistent extraction
./isosurface_renderer_app -f <volume.raw> --task_mesh_persistent

# build a reference meshlet set offline (uses meshoptimizer by Arseny Kapoulkine)
./isosurface_renderer_app -f <volume.raw> --reference_meshlets

# run the performance evaluation across the configured viewport sizes
./isosurface_renderer_app -f <volume.raw> --evaluation_mode

# evaluate per-component pipeline timings
./isosurface_renderer_app -f <volume.raw> --per_timing_component_evaluation

# evaluate per-component memory footprint of the persistent extraction
./isosurface_renderer_app -f <volume.raw> --per_memory_footprint_component_eval

# sweep an isovalue range and log a measurement per step
./isosurface_renderer_app -f <volume.raw> --range_eval --iso_value 0.1 --end_iso_value 0.9 --step_size 0.05

# headless / no-ImGui startup
./isosurface_renderer_app -f <volume.raw> --no_gui

# initial model orientation (degrees, applied as yaw -> pitch -> roll)
./isosurface_renderer_app -f <volume.raw> --m_yaw 45 --m_pitch 30 --m_roll 0
```

## Sizing the extracted-geometry buffers

The persistent extraction pipeline writes vertices, normals and indices into pre-allocated SSBOs. Their sizes are **not** auto-derived from the volume — by default a fallback heuristic is used, which is sufficient for small volumes but **will silently truncate (or, in some configurations, crash) on larger volumes or denser isosurfaces** where the extracted geometry exceeds the default budget.

For any non-trivial volume you should explicitly pass the maximum expected size of each output buffer:

```
./isosurface_renderer_app -f <volume.raw> \
    --position_buffer_MB <N> \
    --normal_buffer_MB   <N> \
    --index_buffer_MB    <M>
```

Sizes are in megabytes. A reasonable starting point is to estimate the worst-case extracted vertex count for PMB `V` and triangle count `T` for your volume + isovalue, then size:
* `--position_buffer_MB` ≥ `V * 5 / 1e6` (3 × `float`)
* `--normal_buffer_MB`   ≥ `V * 5 / 1e6` (3 × `float`)
* `--index_buffer_MB`    ≥ `T * 3 * 4 / 1e6` (32-bit indices; halve for 16-bit-index variants)

If extraction looks incomplete (missing chunks of the surface or entirely missing surface) or the app aborts on a buffer-related error, increase these values. The long forms `--position_buffer_size_in_MB`, `--normal_buffer_size_in_MB`, `--index_buffer_size_in_MB` are also accepted.

## Useful Hotkeys

```
  c     - open camera settings
  r     - toggle volume auto rotation
```

A more complete list, including all CLI flags with their long and short forms, is printed when running the app with `--help`.
