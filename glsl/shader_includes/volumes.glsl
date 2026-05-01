#ifndef VOLUMES_GLSL_INCLUDE
#define VOLUMES_GLSL_INCLUDE

#include "ubos.glsl"

//volume to extract isosurface for
layout (binding = 0) uniform sampler3D volume_in_tex;

uint flatten_id(in uvec3 three_d_index, in uvec3 three_d_resolution) {

    return three_d_index.x
        + three_d_index.y * three_d_resolution.x
        + three_d_index.z * three_d_resolution.x * three_d_resolution.y;
}

uvec3 unflatten_id(in uint one_d_index, in uvec3 three_d_resolution) {

    uint one_d_index_copy = one_d_index;
    uvec3 unflattened_id_to_return;
    unflattened_id_to_return.z = one_d_index_copy / (three_d_resolution.x * three_d_resolution.y);
    one_d_index_copy -= unflattened_id_to_return.z * (three_d_resolution.x * three_d_resolution.y);
    unflattened_id_to_return.y = one_d_index_copy / (three_d_resolution.x);
    unflattened_id_to_return.x = one_d_index_copy % three_d_resolution.x;

    return unflattened_id_to_return;
}

ivec3 volTexSize = textureSize(volume_in_tex, 0);
ivec3 volTexSizeMinusOne = volTexSize - 1;
vec4 one_by_texture_size = vec4(vec3(1.0) / volTexSize, 0.0);


//sample base volume with integer coordinates - uninterpolated (binding point 0)
float sample_volume(ivec3 in_sampling_pos) {


  in_sampling_pos = min(in_sampling_pos, volTexSizeMinusOne);

  return texelFetch(volume_in_tex, in_sampling_pos, 0).r;
}


// central difference gradient computation from original volume
vec3 sample_gradient(vec3 in_sampling_pos) {
  vec3 normalized_sampling_pos = (in_sampling_pos);

  float left_sample = texture(volume_in_tex, normalized_sampling_pos - one_by_texture_size.xww ).r;
  float right_sample = texture(volume_in_tex, normalized_sampling_pos + one_by_texture_size.xww).r;

  float bottom_sample = texture(volume_in_tex, normalized_sampling_pos - one_by_texture_size.wyw).r;
  float top_sample = texture(volume_in_tex, normalized_sampling_pos + one_by_texture_size.wyw).r;

  float front_sample = texture(volume_in_tex, normalized_sampling_pos - one_by_texture_size.wwz).r;
  float back_sample = texture(volume_in_tex, normalized_sampling_pos + one_by_texture_size.wwz).r;

  return normalize(vec3(left_sample-right_sample, bottom_sample-top_sample, front_sample-back_sample));
}


#endif //VOLUMES_GLSL_INCLUDE
