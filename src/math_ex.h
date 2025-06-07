#pragma once

#include <stdbool.h>

typedef struct aabb
{
    struct
    {
        float x;
        float z;
    }
    min, max;
}
aabb_t;

typedef struct transform
{
    struct
    {
        float x;
        float y;
        float z;
    }
    position;
    float rotation;
}
transform_t;

typedef enum camera_mode
{
    camera_mode_perspective,
    camera_mode_ortho_2d,
    camera_mode_ortho_3d,

    camera_mode_count,
}
camera_mode_t;

typedef struct camera
{
    camera_mode_t mode;

    float matrix[4][4];

    float tx;
    float tz;

    float px;
    float py;
    float pz;

    float pitch;
    float yaw;
    float distance;

    float width;
    float height;

    float fov;
    float near;
    float far;

    float speed;
}
camera_t;

void camera_init(camera_t* camera, camera_mode_t mode);
void camera_update(camera_t* camera, float x, float z, float dt, bool frustum);
void camera_get_vector(const camera_t* camera, float* x, float* y, float* z);