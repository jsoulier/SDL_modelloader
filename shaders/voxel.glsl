#ifndef VOXEL_GLSL
#define VOXEL_GLSL
    
/*
 * 00-00: x sign bit
 * 01-07: x (7 bits)
 * 08-08: y sign bit
 * 09-15: y (7 bits)
 * 16-16: z sign bit
 * 17-23: z (7 bits)
 * 24-26: direction (3 bits)
 * 27-31: unused (5 bits)
 * 32-63: texcoord (32 bits)
 */

vec3 voxel_get_position(uint packed)
{
    int sign_x = 1 - 2 * int((packed >> 7)  & 0x1);
    int sign_y = 1 - 2 * int((packed >> 15) & 0x1);
    int sign_z = 1 - 2 * int((packed >> 23) & 0x1);

    int x = int((packed >>  0) & 0x7F);
    int y = int((packed >>  8) & 0x7F);
    int z = int((packed >> 16) & 0x7F);

    return vec3(x * sign_x, y * sign_y, z * sign_z);
}

vec3 voxel_get_normal(uint packed)
{
    const vec3 directions[6] = vec3[]
    (
        vec3( 1.0f, 0.0f, 0.0f),
        vec3(-1.0f, 0.0f, 0.0f),
        vec3( 0.0f, 1.0f, 0.0f),
        vec3( 0.0f,-1.0f, 0.0f),
        vec3( 0.0f, 0.0f, 1.0f),
        vec3( 0.0f, 0.0f,-1.0f)
    );

    int direction = int((packed >> 24) & 0x7);
    return directions[direction];
}

#endif