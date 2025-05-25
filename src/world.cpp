#include <array>
#include <memory>
#include <vector>

#include "entity.hpp"
#include "tile.hpp"
#include "world.hpp"

struct Floor
{
    std::vector<std::shared_ptr<Entity>> entities;
};

static std::array<Floor, floor_count> floors;

static FloorId current_floor;

bool init_world()
{
    return true;
}

void shutdown_world()
{

}

void update_world(float dt)
{
    for (auto& entity : floors[current_floor].entities)
    {
        entity->update(dt);
    }
}

void render_world()
{
    for (auto& entity : floors[current_floor].entities)
    {
        entity->render();
    }
}