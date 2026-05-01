#version 460

layout (location = 0) in Interpolant {
//0 -> world space x
//1 -> world space y
//2 -> world space z
flat uint8_t axis_id;
} IN;

layout (location = 0) out vec4 FragColor;

const vec3 axes_colors[3] = {
  {1.0f, 0.0f, 0.0f},
  {0.0f, 1.0f, 0.0f},
  {0.0f, 0.0f, 1.0f}
};

void main() {
    FragColor = vec4(axes_colors[IN.axis_id], 1.0f);
}