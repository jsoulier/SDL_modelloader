#include <FastNoiseLite.h>

#include <memory>
#include <print>

#include "entity.hpp"
#include "noise.hpp"
#include "world.hpp"

bool Noise::generate()
{
    std::shared_ptr<Entity> player = Entity::create(ENTITY_PLAYER);
    if (!player)
    {
        std::println("Failed to create player");
        return false;
    }

    World::insert(player, LEVEL_OVERWORLD);

    return true;
}