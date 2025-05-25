#include <memory>
#include <print>

#include "entity.hpp"
#include "entity/drop.hpp"
#include "entity/mob.hpp"
#include "entity/player.hpp"

std::shared_ptr<Entity> create_entity(EntityType type, void* args)
{
    /* TODO: Allocation strategy */

    std::shared_ptr<Entity> entity;
    switch (type)
    {

    case entity_player:
        entity = std::make_shared<PlayerEntity>();
        break;

    case entity_drop:
        if (args)
        {
            entity = std::make_shared<DropEntity>();
        }
        else
        {
            entity = std::make_shared<DropEntity>(*static_cast<Item*>(args));
        }
        break;

    }

    if (!entity)
    {
        std::println("Failed to create entity");
    }

    return entity;
}