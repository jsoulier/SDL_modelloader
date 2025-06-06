#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "data.h"
#include "dbg.h"

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
        assert(false);
    }
    return NULL;
}

const char* get_shader_path(shader_type_t type)
{
    switch (type)
    {
    default:
        assert(false);
    }
    return NULL;
}

const char* get_compute_pipeline_path(compute_pipeline_type_t type)
{
    switch (type)
    {
    default:
        assert(false);
    }
    return NULL;
}

const char* get_texture_path(texture_type_t type)
{
    switch (type)
    {
    default:
        assert(false);
    }
    return NULL;
}

const char* get_font_path(font_type_t type)
{
    switch (type)
    {
    default:
        assert(false);
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
    assert(tile);
    assert(type >= 0);
    assert(type < tile_type_count);

    tile->type = type;
}

void item_init(item_t* item, item_type_t type)
{
    assert(item);
    assert(type >= 0);
    assert(type < item_type_count);

    item->type = type;
}

mesh_type_t tile_get_mesh_type(const tile_t* tile)
{
    assert(tile);

    return tiles[tile->type].mesh_type;
}

mesh_type_t item_get_mesh_type(const item_t* item)
{
    assert(item);

    return mesh_type_count;
}