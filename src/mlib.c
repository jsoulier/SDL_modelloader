#include <float.h>
#include <math.h>
#include <stdbool.h>

#include "dbg.h"
#include "mlib.h"

static const float pi = 3.1415927f;
static const float epsilon = FLT_EPSILON;

static float to_radians(float degrees)
{
    return degrees * (pi / 180.0f);
}

static float to_degrees(float radians)
{
    return radians * (180.0f / pi);
}

static float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static void mat4_multiply(float matrix[4][4], const float a[4][4], const float b[4][4])
{
    float c[4][4];

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            c[i][j] = 0.0f;
            for (int k = 0; k < 4; k++)
            {
                c[i][j] += a[k][j] * b[i][k];
            }
        }
    }

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            matrix[i][j] = c[i][j];
        }
    }
}

static void mat4_vec3_multiply(float vector[4], const float a[4][4], const float b[4])
{
    float c[4];

    for (int i = 0; i < 4; i++)
    {
        c[i] = 0.0f;
        for (int j = 0; j < 4; j++)
        {
            c[i] += a[j][i] * b[j];
        }
    }

    for (int i = 0; i < 4; i++)
    {
        vector[i] = c[i];
    }
}

static void mat4_translate(float matrix[4][4], float x, float y, float z)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            matrix[i][j] = 0.0f;
        }
    }

    matrix[0][0] = 1.0f;
    matrix[1][1] = 1.0f;
    matrix[2][2] = 1.0f;
    matrix[3][0] = x;
    matrix[3][1] = y;
    matrix[3][2] = z;
    matrix[3][3] = 1.0f;
}

static void mat4_rotate(float matrix[4][4], float x, float y, float z, float angle)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            matrix[i][j] = 0.0f;
        }
    }

    float s = sinf(angle);
    float c = cosf(angle);
    float i = 1.0f - c;
    matrix[0][0] = i * x * x + c;
    matrix[0][1] = i * x * y - z * s;
    matrix[0][2] = i * z * x + y * s;
    matrix[1][0] = i * x * y + z * s;
    matrix[1][1] = i * y * y + c;
    matrix[1][2] = i * y * z - x * s;
    matrix[2][0] = i * z * x - y * s;
    matrix[2][1] = i * y * z + x * s;
    matrix[2][2] = i * z * z + c;
    matrix[3][3] = 1.0f;
}

static void mat4_perspective(float matrix[4][4], float fov, float width, float height, float near, float far)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            matrix[i][j] = 0.0f;
        }
    }

    float f = 1.0f / tanf(fov / 2.0f);
    float ratio = width / height;
    matrix[0][0] = f / ratio;
    matrix[1][1] = f;
    matrix[2][2] = -(far + near) / (far - near);
    matrix[2][3] = -1.0f;
    matrix[3][2] = -(2.0f * far * near) / (far - near);
}

static void mat4_ortho(float matrix[4][4], float left, float right, float bottom, float top, float near, float far)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            matrix[i][j] = 0.0f;
        }
    }

    matrix[0][0] = 2.0f / (right - left);
    matrix[1][1] = 2.0f / (top - bottom);
    matrix[2][2] = -1.0f / (far - near);
    matrix[3][0] = -(right + left) / (right - left);
    matrix[3][1] = -(top + bottom) / (top - bottom);
    matrix[3][2] = -near / (far - near);
    matrix[3][3] = 1.0f;
}

static void mat4_inverse(float matrix[4][4], const float z[4][4])
{
    float t[6];
    float
        a = z[0][0],
        b = z[0][1],
        c = z[0][2],
        d = z[0][3],
        e = z[1][0],
        f = z[1][1],
        g = z[1][2],
        h = z[1][3],
        i = z[2][0],
        j = z[2][1],
        k = z[2][2],
        l = z[2][3],
        m = z[3][0],
        n = z[3][1],
        o = z[3][2],
        p = z[3][3];

    t[0] = k * p - o * l;
    t[1] = j * p - n * l;
    t[2] = j * o - n * k;
    t[3] = i * p - m * l;
    t[4] = i * o - m * k;
    t[5] = i * n - m * j;

    matrix[0][0] = f * t[0] - g * t[1] + h * t[2];
    matrix[1][0] = -(e * t[0] - g * t[3] + h * t[4]);
    matrix[2][0] = e * t[1] - f * t[3] + h * t[5];
    matrix[3][0] = -(e * t[2] - f * t[4] + g * t[5]);

    matrix[0][1] = -(b * t[0] - c * t[1] + d * t[2]);
    matrix[1][1] = a * t[0] - c * t[3] + d * t[4];
    matrix[2][1] = -(a * t[1] - b * t[3] + d * t[5]);
    matrix[3][1] = a * t[2] - b * t[4] + c * t[5];

    t[0] = g * p - o * h;
    t[1] = f * p - n * h;
    t[2] = f * o - n * g;
    t[3] = e * p - m * h;
    t[4] = e * o - m * g;
    t[5] = e * n - m * f;

    matrix[0][2] = b * t[0] - c * t[1] + d * t[2];
    matrix[1][2] = -(a * t[0] - c * t[3] + d * t[4]);
    matrix[2][2] = a * t[1] - b * t[3] + d * t[5];
    matrix[3][2] = -(a * t[2] - b * t[4] + c * t[5]);

    t[0] = g * l - k * h;
    t[1] = f * l - j * h;
    t[2] = f * k - j * g;
    t[3] = e * l - i * h;
    t[4] = e * k - i * g;
    t[5] = e * j - i * f;

    matrix[0][3] = -(b * t[0] - c * t[1] + d * t[2]);
    matrix[1][3] = a * t[0] - c * t[3] + d * t[4];
    matrix[2][3] = -(a * t[1] - b * t[3] + d * t[5]);
    matrix[3][3] = a * t[2] - b * t[4] + c * t[5];

    float determinant =
        a * matrix[0][0] +
        b * matrix[1][0] +
        c * matrix[2][0] +
        d * matrix[3][0];

    if (determinant < epsilon)
    {
        log_release("Determinant approaching zero");
        return;
    }

    for (int s = 0; s < 4; s++)
    {
        for (int r = 0; r < 4; r++)
        {
            matrix[s][r] /= determinant;
        }
    }
}

void camera_init(camera_t* camera, camera_mode_t mode)
{
    assert(camera);

    camera->mode = mode;

    camera->tx = 0.0f;
    camera->tz = 0.0f;

    camera->px = 0.0f;
    camera->py = 0.0f;
    camera->pz = 0.0f;

    camera->pitch = 0.0f;
    camera->yaw = 0.0f;
    camera->distance = 0.0f;

    camera->width = 0.0f;
    camera->height = 0.0f;

    camera->fov = to_radians(60.0f);
    camera->near = 1.0f;
    camera->far = 1000.0f;

    camera->speed = 1.0f;
}

void camera_update(camera_t* camera, float x, float z, float dt, bool frustum)
{
    assert(camera);

    float width = fmaxf(camera->width, 1.0f);
    float height = fmaxf(camera->height, 1.0f);

    if (camera->mode == camera_mode_ortho_2d)
    {
        mat4_ortho(camera->matrix, 0.0f, width, 0.0f, height, -1.0f, 1.0f);
        return;
    }

    float t = fminf(dt * camera->speed, 1.0f);
    camera->tx = lerp(camera->tx, x, t);
    camera->tz = lerp(camera->tz, z, t);

    float fx;
    float fy;
    float fz;

    camera_get_vector(camera, &fx, &fy, &fz);

    camera->px = camera->tx - fx * camera->distance;
    camera->py = 0.0f - fy * camera->distance;
    camera->pz = camera->tz - fz * camera->distance;

    float a[4][4];
    float b[4][4];

    mat4_translate(a, -camera->px, -camera->py, -camera->pz);

    mat4_rotate(b, cosf(camera->yaw), 0.0f, sinf(camera->yaw), camera->pitch);
    mat4_multiply(a, b, a);

    mat4_rotate(b, 0.0f, 1.0f, 0.0f, -camera->yaw);
    mat4_multiply(a, b, a);

    if (camera->mode == camera_mode_perspective)
    {
        mat4_perspective(b, camera->fov, width, height, camera->near, camera->far);
    }
    else if (camera->mode == camera_mode_ortho_3d)
    {
        mat4_ortho(b, -width / 2.0f, width / 2.0f, -height / 2.0f, height / 2.0f, -camera->far, camera->far);
    }
    else
    {
        assert(false);
    }

    mat4_multiply(camera->matrix, b, a);

    if (frustum)
    {
        return;
    }

    mat4_inverse(b, camera->matrix);

    /* TODO: */
}

void camera_get_vector(const camera_t* camera, float* x, float* y, float* z)
{
    assert(camera);

    assert(x);
    assert(y);
    assert(z);

    *x = cosf(camera->yaw) * cosf(camera->pitch);
    *y = sinf(camera->pitch);
    *z = sinf(camera->yaw) * cosf(camera->pitch);
}