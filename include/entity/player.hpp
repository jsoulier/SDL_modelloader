#pragma once

#include "entity/mob.hpp"

class PlayerEntity : public MobEntity
{
public:
    glm::vec3 get_motion() const override;
    ModelId get_model() const override;
    EntityType get_type() const override;
};