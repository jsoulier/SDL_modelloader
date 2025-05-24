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

bool init_world()
{
    return true;
}

void shutdown_world()
{

}