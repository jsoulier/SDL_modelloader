#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

struct transform_component
{
    glm::vec3 position;
    float yaw;
};

extern entt::registry registry;