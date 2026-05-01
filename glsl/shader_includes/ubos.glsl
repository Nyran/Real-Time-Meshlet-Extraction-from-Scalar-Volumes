#ifndef UBO_GLSL_INCLUDE
#define UBO_GLSL_INCLUDE

layout(std140, binding = 4) uniform main_camera_uniform_buffer_object
{
    mat4 camera_mat;
    mat4 view_mat;
    mat4 projection_mat;
    mat4 inverse_projection_mat;
    mat4 view_projection_mat;
    mat4 inverse_view_projection_mat;
    vec4 pos_world_space;

    //computation as in: https://github.com/zeux/niagara/blob/master/src/niagara.cpp
    vec4 frustum_planes;
    vec4 near_far_padding_x2;
} culling_reference_camera_UBO;

layout(std140, binding = 5) uniform point_of_view_camera_uniform_buffer_object
{
    mat4 camera_mat;
    mat4 view_mat;
    mat4 projection_mat;
    mat4 inverse_projection_mat;
    mat4 view_projection_mat;
    mat4 inverse_view_projection_mat;
    vec4 pos_world_space;

    //computation as in: https://github.com/zeux/niagara/blob/master/src/niagara.cpp
    vec4 frustum_planes;
    vec4 near_far_padding_x2;
} point_of_view_camera_UBO;

layout(std140, binding = 6) uniform model_uniform_buffer_object
{
    mat4 model_mat;
    mat4 inv_model_mat;
} model_UBO;

layout(std140, binding = 7) uniform combined_model_main_camera_uniform_buffer_object
{
    mat4 model_view_mat;
    mat4 inverse_model_view_mat;
    mat4 normal_mat;
    mat4 model_view_projection_mat;

    vec4 vol_space_cam_pos;
} combined_model_culling_reference_camera_UBO;

layout(std140, binding = 8) uniform combined_model_point_of_view_camera_uniform_buffer_object
{
    mat4 model_view_mat;
    mat4 inverse_model_view_mat;
    mat4 normal_mat;
    mat4 model_view_projection_mat;

    vec4 vol_space_cam_pos;
} combined_model_point_of_view_camera_UBO;

layout(std140, binding = 12) uniform draw_parameters_uniform_buffer_object
{
    vec4 shading_base_color_rgba;
} draw_parameters_UBO;

layout(std140, binding = 13) uniform extraction_parameters_uniform_buffer_object
{
    vec4 voxel_size;
    ivec4 volume_resolution;
    float iso_value;
} extraction_parameters_UBO;

layout(std140, binding = 14) uniform fbo_resident_texture_handles_uniform_buffer_object
{
    uvec4 color_and_depth_fbo_handles;
} fbo_resident_texture_handles_UBO;

layout(std140, binding = 15) uniform frame_count_dependent_variables_ubo{
    uint counter;
} frame_descriptor_UBO;

#endif //UBO_GLSL_INCLUDE
