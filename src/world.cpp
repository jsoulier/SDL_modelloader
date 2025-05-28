#include <array>
#include <memory>
#include <print>
#include <vector>

#include "config.hpp"
#include "database.hpp"
#include "entity.hpp"
#include "renderer.hpp"
#include "tile.hpp"
#include "world.hpp"
#include "world.hpp"

struct Level
{
    std::vector<std::shared_ptr<Entity>> entities;
    std::array<std::array<Tile, WORLD_WIDTH>, WORLD_WIDTH> tiles;
};

static std::array<Level, LEVEL_COUNT> levels;
static std::shared_ptr<Entity> player;
static glm::ivec2 min_tiles;
static glm::ivec2 max_tiles;

static LevelId current_level;

bool World::init()
{
    Database::select(insert);

    return true;
}

void World::shutdown()
{

}

std::shared_ptr<Entity> World::get_player()
{
    return player;
}

void World::move()
{

}

void World::update(float dt)
{
    for (auto& entity : levels[current_level].entities)
    {
        entity->update(dt);
    }
}

void World::draw()
{
    for (auto& entity : levels[current_level].entities)
    {
        Renderer::draw(entity->get_model(), entity->get_transform());
    }
}

void World::commit()
{
    for (auto& entity : levels[current_level].entities)
    {
        Database::insert(entity, current_level);
    }
}

void World::insert(std::shared_ptr<Entity>& entity, LevelId level)
{
    if (entity->get_type() == ENTITY_PLAYER)
    {
        if (player)
        {
            std::println("Tried to add multiple players");
            return;
        }

        player = entity;
        current_level = level;
    }

    if (level == LEVEL_COUNT)
    {
        levels[current_level].entities.push_back(entity);
    }
    else
    {
        levels[level].entities.push_back(entity);
    }

    Database::insert(entity, level);
}