#include <limits>
#include <cmath>

#include "entity/mob.hpp"
#include "math.hpp"
#include "renderer.hpp"

void MobEntity::update(float dt)
{
    glm::vec3 motion = get_motion();

    float length = glm::length(motion);
    if (length < std::numeric_limits<float>::epsilon())
    {
        return;
    }

    motion /= length;

    static constexpr float speed = 100.0f;

    transform.position += motion * speed * dt;
    transform.rotation = std::atan2(motion.y, motion.z);
}

void MobEntity::render() const
{
    Renderer::draw(get_model(), get_transform());
}