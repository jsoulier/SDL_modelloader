#include "renderer.hpp"
#include "tile.hpp"

struct TileData
{
    ModelId model;
};

/* TODO: convert to std::array */

static const constexpr TileData TILES[TILE_COUNT] =
{{
    MODEL_INVALID,
},
{
    MODEL_GRASS,
},
{
    MODEL_DIRT,
},
{
    MODEL_TREE,
},
{
    MODEL_SAND,
},
{
    MODEL_WATER,
}};

Tile::Tile(TileType type)
    : type{type}
{

}

ModelId Tile::get_model() const
{
    return TILES[type].model;
}