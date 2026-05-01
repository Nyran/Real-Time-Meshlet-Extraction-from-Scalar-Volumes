#version 460


#include "../shader_includes/ubos.glsl"
#include "../shader_includes/shading.glsl"

layout(early_fragment_tests) in;

layout(location = 0) in Interpolant {
vec3 ws_position;
vec3 ws_normal;
} IN;

layout(location = 0) out vec4 FragColor;

void main() {

  


vec3 per_fragment_ws_light_dir = normalize(ws_light_pos - IN.ws_position.xyz);
float lambertian_intensity = max(0.0f, dot(per_fragment_ws_light_dir, IN.ws_normal.xyz));

vec3 per_fragment_ws_view_dir = normalize(point_of_view_camera_UBO.pos_world_space.xyz-IN.ws_position.xyz);
vec3 half_dir = normalize(per_fragment_ws_light_dir + per_fragment_ws_view_dir);

float specular_cosine_similarity = max(0.0f, dot(half_dir, IN.ws_normal.xyz));
float specular_intensity = pow(specular_cosine_similarity, 30.0f);

vec4 base_color_to_apply = draw_parameters_UBO.shading_base_color_rgba;
FragColor = (
			vec4(0.2f * base_color_to_apply.rgb, 1.0f) +
			vec4(base_color_to_apply.rgb * lambertian_intensity, 1.0f) +
			vec4( (base_color_to_apply.rgb + vec3(0.2f)) * specular_intensity, 1.0f) );
}