#pragma once

#include <memory>

#include "math.hpp"

enum EntityType
{
    /* Don't change the order! */
    ENTITY_PLAYER,
    ENTITY_DROP,
};

class Entity
{
public:
    Entity()
        {}

    static std::shared_ptr<Entity> create(EntityType type, void* args = nullptr);
    virtual void update(float dt) = 0;
    virtual void render() const = 0;
    virtual EntityType get_type() const = 0;
    const Transform& get_transform() const;

protected:
    Transform transform;
};