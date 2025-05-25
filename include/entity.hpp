#pragma once

#include <memory>

enum EntityType
{
    /* Don't change the order! */
    entity_player,
    entity_drop,
};

class Entity
{
public:
    Entity()
        {}

    virtual void update(float dt) = 0;
    virtual void render() const = 0;

protected:

};

std::shared_ptr<Entity> create_entity(EntityType type, void* args);