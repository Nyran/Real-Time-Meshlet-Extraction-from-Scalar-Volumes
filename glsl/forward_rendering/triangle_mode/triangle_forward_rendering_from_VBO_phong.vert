#version 460

#include "../../shader_includes/ubos.glsl"
#include "../../shader_includes/ssbos.glsl"

layout(location = 0) out Interpolant {
    vec3 ws_position;
    vec3 ws_normal;
} OUT;


void main() {

    uint v_id = uint(gl_VertexID); // already includes baseVertex in GL

  vec3 looked_up_position = read_extracted_vertex_position(v_id);
  vec3 looked_up_normal= read_extracted_vertex_normals(v_id);

  const vec4 homogeneous_tri_vertex_position = vec4(looked_up_position, 1.0);
  gl_Position =  combined_model_point_of_view_camera_UBO.model_view_projection_mat * homogeneous_tri_vertex_position;


  OUT.ws_position = (model_UBO.model_mat * homogeneous_tri_vertex_position).xyz;
  OUT.ws_normal = normalize((combined_model_point_of_view_camera_UBO.normal_mat * vec4(looked_up_normal, 0.0) ).xyz);

}