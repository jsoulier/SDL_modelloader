#pragma once

#include "math.hpp"

enum CameraType
{
    CAMERA_PERSPECTIVE,
    CAMERA_ORTHO_3D,
};

class Camera
{
public:
    Camera()
        : type{CAMERA_PERSPECTIVE}
        , matrix{}
        , position{}
        , target{}
        , distance{0.0f}
        , fov{glm::radians(45.0f)}
        , width{960.0f}
        , height{720.0f}
        , near{0.1f}
        , far{1000.0f}
        , pitch{0.0f}
        , yaw{0.0f}
        , speed{1.0f}
        {}

    void update(float dt);

    void set_type(CameraType type);
    void set_target(const glm::vec3& target);
    void set_distance(float distance);
    void set_fov(float fov);
    void set_width(float width);
    void set_height(float height);
    void set_near(float near);
    void set_far(float far);
    void set_pitch(float pitch);
    void set_yaw(float yaw);
    void set_speed(float speed);

    const glm::mat4& get_matrix() const;
    const glm::vec3& get_position() const;
    const glm::vec3& get_target() const;
    const glm::vec2& get_min() const;
    const glm::vec2& get_max() const;

private:
    CameraType type;

    glm::mat4 matrix;
    glm::vec3 position;
    glm::vec3 target;

    float distance;
    float fov;
    float width;
    float height;
    float near;
    float far;
    float pitch;
    float yaw;
    float speed;

    glm::vec2 min;
    glm::vec2 max;
};