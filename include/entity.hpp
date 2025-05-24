#pragma once

#include <memory>

enum EntityType
{
    /* Don't change the order! */
    entity_player,
};

class Entity
{
public:
    Entity()
        {}
};

std::shared_ptr<Entity> create_entity(EntityType type, void* args);