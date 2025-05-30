#version 450

layout(location = 0) in vec3 i_position;
layout(location = 1) in flat vec2 i_uv;
layout(location = 2) in flat vec3 i_normal;

layout(location = 0) out vec4 o_position;
layout(location = 1) out vec4 o_color;
layout(location = 2) out vec4 o_normal;

layout(set = 2, binding = 0) uniform sampler2D s_palette;

void main()
{
    o_position = vec4(i_position, 0.0f);
    o_color = texture(s_palette, i_uv);
    o_normal = vec4(i_normal, 0.0f);
}