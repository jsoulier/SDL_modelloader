#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "data.h"
#include "util.h"

const char* get_mesh_path(mesh_type_t type)
{
    switch (type)
    {
    case mesh_type_player_00:
        return "player_00";
    case mesh_type_dirt_00:
        return "dirt_00";
    case mesh_type_grass_00:
        return "grass_00";
    case mesh_type_sand_00:
        return "sand_00";
    case mesh_type_tree_00:
        return "tree_00";
    case mesh_type_water_00:
        return "water_00";
    default:
        assert_debug(false);
    }

    return NULL;
}

const char* get_shader_path(shader_type_t type)
{
    switch (type)
    {
    case shader_type_composite_frag:
        return "composite.frag";
    case shader_type_mesh_frag:
        return "mesh.frag";
    case shader_type_mesh_vert:
        return "mesh.vert";
    case shader_type_screen_vert:
        return "screen.vert";
    default:
        assert_debug(false);
    }

    return NULL;
}

const char* get_compute_pipeline_path(compute_pipeline_type_t type)
{
    switch (type)
    {
    case compute_pipeline_type_readback:
        return "readback.comp";
    default:
        assert_debug(false);
    }

    return NULL;
}

struct
{
    mesh_type_t mesh_type;
}
static const tiles[tile_type_count] =
{
    [tile_type_dirt] =
    {
        .mesh_type = mesh_type_dirt_00,
    },
    [tile_type_grass] =
    {
        .mesh_type = mesh_type_grass_00,
    },
    [tile_type_sand] =
    {
        .mesh_type = mesh_type_sand_00,
    },
    [tile_type_tree] =
    {
        .mesh_type = mesh_type_tree_00,
    },
    [tile_type_water] =
    {
        .mesh_type = mesh_type_water_00,
    },
};

void tile_init(tile_t* tile, tile_type_t type)
{
    assert_debug(tile);
    assert_debug(type >= 0);
    assert_debug(type < tile_type_count);

    tile->type = type;
}

void item_init(item_t* item, item_type_t type)
{
    assert_debug(item);
    assert_debug(type >= 0);
    assert_debug(type < item_type_count);

    item->type = type;
}

mesh_type_t tile_get_mesh_type(const tile_t* tile)
{
    assert_debug(tile);

    return tiles[tile->type].mesh_type;
}

mesh_type_t item_get_mesh_type(const item_t* item)
{
    assert_debug(item);

    return mesh_type_count;
}