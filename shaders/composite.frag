#version 450

layout(location = 0) in vec2 i_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D s_color;
layout(set = 2, binding = 1) uniform sampler2D s_position;
layout(set = 2, binding = 2) uniform sampler2D s_normal;

void main()
{
    o_color = texelFetch(s_color, ivec2(gl_FragCoord.xy), 0);

    /* TODO */
    o_color.a = 1.0f;
}