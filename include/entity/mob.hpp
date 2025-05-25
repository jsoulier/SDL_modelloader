#pragma once

#include "entity.hpp"

class MobEntity : public Entity
{
public:
    void update(float dt) override;
    void render() const override;
};