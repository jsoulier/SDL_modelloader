#include <cassert>
#include <cmath>
#include <limits>

#include "camera.hpp"
#include "math.hpp"

void Camera::update(float dt)
{
    static constexpr glm::vec3 up{0.0f, 1.0f, 0.0f};

    glm::vec3 forward;
    forward.x = std::cos(yaw) * std::cos(pitch);
    forward.y = std::sin(pitch);
    forward.z = std::sin(yaw) * std::cos(pitch);
    forward = glm::normalize(forward);

    // float a = 1.0f - std::exp(-speed * dt);
    float a = 1.0f;
    position = glm::mix(position, target - forward * distance, a);

    glm::mat4 view = glm::lookAt(position, position + forward, up);
    glm::mat4 proj;

    switch (type)
    {
    case camera_type_perspective:
        proj = glm::perspective(fov, width / height, near, far);
        break;
    case camera_type_ortho_3d:
        proj = glm::ortho(0.0f, width, height, 0.0f);
        break;
    default:
        assert(false);
    }

    matrix = proj * view;

    glm::mat4 inv_matrix = glm::inverse(matrix);
    glm::vec2 corners[4];

    corners[0] = {-1.0f,-1.0f};
    corners[1] = { 1.0f,-1.0f};
    corners[2] = {-1.0f, 1.0f};
    corners[3] = { 1.0f, 1.0f};

    min = glm::vec2{std::numeric_limits<float>::min()};
    max = glm::vec2{std::numeric_limits<float>::max()};

    for (glm::vec2& corner : corners)
    {
        glm::vec4 near = {corner.x, corner.y, 0.0f, 1.0f};
        glm::vec4 far = {corner.x, corner.y, 1.0f, 1.0f};

        near = inv_matrix * near;
        far = inv_matrix * far;

        near /= near.w;
        far /= far.w;

        glm::vec3 direction = far - near;

        static constexpr float intersection = 0.0f;

        float t = (intersection - near.y) / direction.y;

        corner.x = near.x + t * direction.x;
        corner.y = near.z + t * direction.z;

        max = glm::max(max, corner);
        min = glm::min(min, corner);
    }
}

void Camera::set_type(CameraType type)
{
    this->type = type;
}

void Camera::set_target(const glm::vec3& target)
{
    this->target = target;
}

void Camera::set_distance(float distance)
{
    this->distance = distance;
}

void Camera::set_fov(float fov)
{
    this->fov = glm::radians(fov);
}

void Camera::set_width(float width)
{
    this->width = width;
}

void Camera::set_height(float height)
{
    this->height = height;
}

void Camera::set_near(float near)
{
    this->near = near;
}

void Camera::set_far(float far)
{
    this->far = far;
}

void Camera::set_pitch(float pitch)
{
    this->pitch = glm::radians(pitch);
}

void Camera::set_yaw(float yaw)
{
    this->yaw = glm::radians(yaw);
}

void Camera::set_speed(float speed)
{
    this->speed = speed;
}

const glm::mat4& Camera::get_matrix() const
{
    return matrix;
}

const glm::vec3& Camera::get_target() const
{
    return target;
}

const glm::vec2& Camera::get_min() const
{
    return min;
}

const glm::vec2& Camera::get_max() const
{
    return max;
}