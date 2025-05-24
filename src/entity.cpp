#include <memory>
#include <print>

#include "entity.hpp"
#include "entity/mob.hpp"
#include "entity/player.hpp"

std::shared_ptr<Entity> create_entity(EntityType type, void* args)
{
    /* TODO: Allocation strategy */

    std::shared_ptr<Entity> entity;
    switch (type)
    {

    }

    if (!entity)
    {
        std::println("Failed to create entity");
    }

    return entity;
}