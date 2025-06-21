#pragma once

#include <cstdint>

static constexpr int MppTileWidth = 16;

static constexpr int MppTileGrass = 0;
static constexpr int MppTileDirt = 1;
static constexpr int MppTileSand = 2;
static constexpr int MppTileCount = 3;

class MppTile
{
public:
    MppTile() : type(-1) {}
    MppTile(int type);
    void render(int x, int y);
    void update(int x, int y, uint64_t dt, uint64_t time);
    bool operator==(const MppTile& other) const;
    bool operator!=(const MppTile& other) const;
    operator bool() const;

private:
    int type;
};