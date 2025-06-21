#include <SDL3/SDL.h>

#include <cassert>

#include "level.hpp"
#include "renderer.hpp"
#include "tile.hpp"

struct
{
    int color1;
    int color2;
    int color3;
    int color4;
}
static constexpr Tiles[] =
{
    /* grass */
    {
        .color1 = 140,
        .color2 = 230,
        .color3 = 250,
        .color4 = 420,
    },
    /* dirt */
    {
        .color1 = 420,
        .color2 = 420,
        .color3 = 420,
        .color4 = 420,
    },
    /* sand */
    {
        .color1 = 550,
        .color2 = 440,
        .color3 = 440,
        .color4 = 420,
    },
};

static_assert(SDL_arraysize(Tiles) == MppTileCount);

/**
 * 0: north
 * 1: south
 * 2: east
 * 3: west
 */
static constexpr int Connections[][2] =
{
    /* 0b0000 */ {15, 0},
    /* 0b0001 */ {14, 0},
    /* 0b0010 */ {13, 0},
    /* 0b0011 */ {9, 0},
    /* 0b0100 */ {11, 0},
    /* 0b0101 */ {2, 0},
    /* 0b0110 */ {1, 0},
    /* 0b0111 */ {5, 0},
    /* 0b1000 */ {12, 0},
    /* 0b1001 */ {3, 0},
    /* 0b1010 */ {4, 0},
    /* 0b1011 */ {6, 0},
    /* 0b1100 */ {10, 0},
    /* 0b1101 */ {8, 0},
    /* 0b1110 */ {7, 0},
    /* 0b1111 */ {0, 0},
};

MppTile::MppTile(int type)
    : type{type} {}

void MppTile::render(int x, int y)
{
    assert(type != -1);

    int north = *this == mppLevelGetTile(x, y - 1);
    int south = *this == mppLevelGetTile(x, y + 1);
    int east = *this == mppLevelGetTile(x + 1, y);
    int west = *this == mppLevelGetTile(x - 1, y);

    int hash = 0;
    hash |= north << 0;
    hash |= south << 1;
    hash |= east << 2;
    hash |= west << 3;

    uint64_t sprite = mppRendererCreateSprite(
        Tiles[type].color1,
        Tiles[type].color2,
        Tiles[type].color3,
        Tiles[type].color4,
        Connections[hash][0] * MppTileWidth,
        Connections[hash][1] * MppTileWidth,
        MppTileWidth);

    mppRendererDraw(sprite, x * MppTileWidth, y * MppTileWidth);
}

void MppTile::update(int x, int y, uint64_t dt, uint64_t time)
{
    assert(type != -1);
}

bool MppTile::operator==(const MppTile& other) const
{
    return type == other.type;
}

bool MppTile::operator!=(const MppTile& other) const
{
    return !operator==(other);
}

MppTile::operator bool() const
{
    return type != -1;
}