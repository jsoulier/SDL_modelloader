#pragma once

#include <memory>

#include "math.hpp"
#include "renderer.hpp"
#include "uuid.hpp"

class Serializer;

enum EntityType
{
    /* don't change the order! */
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
    virtual void serialize(Serializer& serializer);

    UUID get_uuid() const;
    void set_uuid(const UUID uuid);

    const Transform& get_transform() const;
    virtual ModelId get_model() const = 0;
    virtual EntityType get_type() const = 0;

protected:
    Transform transform;
    UUID uuid;
};