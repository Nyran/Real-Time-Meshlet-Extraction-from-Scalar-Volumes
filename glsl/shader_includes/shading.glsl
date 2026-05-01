const vec3 ws_light_dir = normalize(vec3(1.0, 1.0, 1.0));
const vec3 ws_light_pos = vec3(0.0, 0.0, 4.5);

// gooch shading used by the different rendering modes
vec3 shade_gooch(vec3 world_space_light_dir, vec3 world_space_normal) {
	float light_intensity = dot(world_space_light_dir, world_space_normal);
	float kd = 1;
	float a = 0.2;
	float b = 0.6;
	vec3 base_color = draw_parameters_UBO.shading_base_color_rgba.rgb;
	float it = ((1.0 + light_intensity) / 2.0);
	return (1-it) * (vec3(0, 0, 0.4) + a*base_color.xyz) 
		   +  it * (vec3(0.4, 0.4, 0) + b*base_color.xyz);
}


vec3 rgb_to_luma(vec3 in_rgb) {

	//https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
	vec3 color_conversion_weights = vec3(0.2126, 0.7152, 0.0722);
	return vec3(dot(color_conversion_weights, in_rgb));
}