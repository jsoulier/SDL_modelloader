#include <array>
#include <memory>
#include <vector>

#include "entity.hpp"
#include "tile.hpp"
#include "world.hpp"

struct Level
{
    std::vector<std::shared_ptr<Entity>> entities;
};

static std::array<Level, LEVEL_COUNT> levels;
static std::shared_ptr<Entity> player;

static LevelId level;

bool World::init()
{
    player = Entity::create(ENTITY_PLAYER);

    levels[level].entities.push_back(player);

    return true;
}

void World::shutdown()
{

}

std::shared_ptr<Entity> World::get_player()
{
    return player;
}

void World::update(float dt)
{
    for (auto& entity : levels[level].entities)
    {
        entity->update(dt);
    }
}

void World::draw()
{
    for (auto& entity : levels[level].entities)
    {
        entity->render();
    }
}