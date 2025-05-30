#version 450

#include "mesh.glsl"

layout(location = 0) in uint i_packed;
layout(location = 1) in float i_texcoord;
layout(location = 2) in vec3 i_position;
layout(location = 3) in float i_rotation;

layout(location = 0) out vec3 o_position;
layout(location = 1) out flat vec2 o_uv;
layout(location = 2) out flat vec3 o_normal;

layout(set = 1, binding = 0) uniform t_matrix
{
    mat4 u_matrix;
};

void main()
{
    float cos_rotation = cos(i_rotation);
    float sin_rotation = sin(i_rotation);

    mat3 rotation = mat3(
        vec3( cos_rotation, 0.0f, sin_rotation),
        vec3(    0.0f, 1.0f,    0.0f),
        vec3(-sin_rotation, 0.0f, cos_rotation)
    );

    o_position = i_position + rotation * mesh_get_position(i_packed);
    o_uv = vec2(i_texcoord, 0.5f);
    o_normal = normalize(rotation * mesh_get_normal(i_packed));

    gl_Position = u_matrix * vec4(o_position, 1.0f);
}