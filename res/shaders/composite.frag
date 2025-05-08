#version 450

layout(location = 0) in vec2 i_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D s_color;
layout(set = 2, binding = 1) uniform sampler2D s_position;
layout(set = 2, binding = 2) uniform sampler2D s_normal;

void main()
{
    const vec4 color = texture(s_color, i_uv);
    const vec3 position = texture(s_position, i_uv).xyz;
    const vec3 normal = texture(s_normal, i_uv).xyz;
    o_color = color;
}