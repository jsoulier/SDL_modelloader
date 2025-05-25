#include <memory>
#include <print>

#include "entity.hpp"
#include "entity/drop.hpp"
#include "entity/mob.hpp"
#include "entity/player.hpp"

std::shared_ptr<Entity> Entity::create(EntityType type, void* args)
{
    /* TODO: Allocation strategy */

    std::shared_ptr<Entity> entity;
    switch (type)
    {

    case ENTITY_PLAYER:
        entity = std::make_shared<PlayerEntity>();
        break;

    case ENTITY_DROP:
        if (args)
        {
            entity = std::make_shared<DropEntity>(*static_cast<Item*>(args));
        }
        else
        {
            entity = std::make_shared<DropEntity>();
        }
        break;

    }

    if (!entity)
    {
        std::println("Failed to create entity");
    }

    return entity;
}

const Transform& Entity::get_transform() const
{
    return transform;
}