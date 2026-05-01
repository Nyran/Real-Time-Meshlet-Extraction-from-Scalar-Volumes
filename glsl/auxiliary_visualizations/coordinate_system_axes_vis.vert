#version 460 core



#include "../shader_includes/ubos.glsl"

layout (location = 0) out Interpolant {
//0 -> world space x
//1 -> world space y
//2 -> world space z
flat uint8_t axis_id;
} OUT;

const vec4 coordinate_axis_vertices[4] = {
  {0.0f, 0.0f, 0.0f, 1.0f},
  {1.0f, 0.0f, 0.0f, 1.0f},
  {0.0f, 1.0f, 0.0f, 1.0f},
  {0.0f, 0.0f, 1.0f, 1.0f}
};

void main() {  
	const uint32_t line_idx = gl_VertexID / 2;
	const uint32_t vertex_of_line_idx = gl_VertexID % 2;
	const uint32_t vertex_lookup_idx = (0 == vertex_of_line_idx) ? (0) : (1 + line_idx);

	gl_Position = point_of_view_camera_UBO.view_projection_mat * coordinate_axis_vertices[vertex_lookup_idx];

	OUT.axis_id = uint8_t(line_idx);
}