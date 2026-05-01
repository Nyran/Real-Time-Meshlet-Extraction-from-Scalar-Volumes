#version 460

layout (location = 0) in Interpolant {
flat uint volume_id;
} IN;

layout (location = 0) out vec4 FragColor;


void main() {
    vec3 frustum_line_color = vec3(0.0f, 0.0f, 0.0f);
    if(1 == IN.volume_id) { // anaglyph left
        frustum_line_color = vec3(1.0f, 0.0f, 0.0f);
    } else if(2 == IN.volume_id) { // anaglyph right
        frustum_line_color = vec3(0.0f, 1.0f, 1.0f);
    }


    #ifdef USES_EXT_MESH_SHADER_EXTENSION
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    #else
    FragColor = vec4(frustum_line_color, 1.0);
    #endif //USES_EXT_MESH_SHADER_EXTENSION
}