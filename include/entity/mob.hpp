#pragma once

#include "entity.hpp"
#include "math.hpp"
#include "renderer.hpp"

class MobEntity : public Entity
{
public:
    void update(float dt) override;
    void render() const override;

protected:
    virtual glm::vec3 get_motion() const = 0;
    virtual ModelId get_model() const = 0;
};