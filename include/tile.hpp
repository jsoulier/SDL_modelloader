#pragma once

enum ModelId;

enum TileType
{
    TILE_INVALID,

    /* don't change the order! */
    TILE_GRASS,
    TILE_DIRT,
    TILE_TREE,
    TILE_SAND,
    TILE_WATER,

    TILE_COUNT,
};

class Tile
{
public:
    Tile(TileType type = TILE_INVALID);
    ModelId get_model() const;

private:
    TileType type;
};