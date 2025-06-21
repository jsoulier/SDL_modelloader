#include <SDL3/SDL.h>

#include <cassert>
#include <cmath>
#include <memory>
#include <vector>

#include "database.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "renderer.hpp"
#include "tile.hpp"

struct Level
{
    std::vector<std::shared_ptr<MppEntity>> entities;
    MppTile tiles[MppLevelWidth][MppLevelWidth];
};

static Level levels[MppLevelDepth];
static int currentLevel;

static std::shared_ptr<MppEntity> player;

static int tileX1;
static int tileY1;
static int tileX2;
static int tileY2;

bool mppLevelInit(bool hasDatabase)
{
    if (hasDatabase)
    {
        mppDatabaseSelect(mppLevelInsert);

        if (!player)
        {
            SDL_Log("Failed to load level: %d", currentLevel);
        }
    }

    if (!player)
    {
        /* TODO: noise */

        player = mppEntityCreate(MppEntityTypePlayer);
        levels[currentLevel].entities.push_back(player);
    }

    /* TODO: */

    for (int x = 0; x < MppLevelWidth; x++)
    for (int y = 0; y < MppLevelWidth; y++)
    {
        levels[currentLevel].tiles[x][y] = MppTile{MppTileGrass};
    }

    levels[0].tiles[1][1] = MppTile{MppTileSand};
    levels[0].tiles[3][1] = MppTile{MppTileSand};
    levels[0].tiles[4][1] = MppTile{MppTileSand};
    levels[0].tiles[5][1] = MppTile{MppTileDirt};
    levels[0].tiles[3][3] = MppTile{MppTileSand};
    levels[0].tiles[2][4] = MppTile{MppTileSand};
    levels[0].tiles[3][4] = MppTile{MppTileSand};
    levels[0].tiles[4][4] = MppTile{MppTileSand};
    levels[0].tiles[3][5] = MppTile{MppTileSand};

    return true;
}

void mppLevelQuit()
{
    for (int i = 0; i < MppLevelDepth; i++)
    {
        levels[i].entities.clear();
    }
}

void mppLevelInsert(std::shared_ptr<MppEntity>& entity)
{
    int level = entity->getLevel();
    if (level == -1)
    {
        assert(currentLevel != -1);
        level = currentLevel;
    }

    if (entity->getType() == MppEntityTypePlayer)
    {
        currentLevel = level;
        player = entity;
    }

    assert(currentLevel != -1);

    levels[currentLevel].entities.push_back(entity);
}

void mppLevelRender()
{
    assert(currentLevel != -1);

    Level& level = levels[currentLevel];

    for (int x = tileX1; x < tileX2; x++)
    for (int y = tileY1; y < tileY2; y++)
    {
        level.tiles[x][y].render(x, y);
    }

    for (std::shared_ptr<MppEntity>& entity : level.entities)
    {
        entity->render();
    }
}

void mppLevelUpdate(uint64_t dt, uint64_t time)
{
    assert(currentLevel != -1);

    Level& level = levels[currentLevel];

    float x1;
    float y1;
    float x2;
    float y2;

    // mppRendererGetCameraBounds(x1, y1, x2, y2);

    x1 = 0.0f;
    y1 = 0.0f;
    x2 = MppLevelWidth;
    y2 = MppLevelWidth;

    // tileX1 = x1 / MppTileWidth;
    // tileY1 = y1 / MppTileWidth;
    // tileX2 = x2 / MppTileWidth;
    // tileY2 = y2 / MppTileWidth;

    tileX1 = x1;
    tileY1 = y1;
    tileX2 = x2;
    tileY2 = y2;

    tileX1 = std::max(tileX1, 0);
    tileY1 = std::max(tileY1, 0);
    tileX2 = std::min(tileX2, MppLevelWidth);
    tileY2 = std::min(tileY2, MppLevelWidth);

    for (int x = tileX1; x < tileX2; x++)
    for (int y = tileY1; y < tileY2; y++)
    {
        level.tiles[x][y].update(x, y, dt, time);
    }

    for (std::shared_ptr<MppEntity>& entity : level.entities)
    {
        entity->update(dt, time);
    }
}

void mppLevelCommit()
{
    assert(currentLevel != -1);

    Level& level = levels[currentLevel];

    for (int x = tileX1; x < tileX2; x++)
    for (int y = tileY1; y < tileY2; y++)
    {
        // mppDatabaseInsert(level.tiles[x][y]);
    }

    for (std::shared_ptr<MppEntity>& entity : level.entities)
    {
        mppDatabaseInsert(entity);
    }
}

std::shared_ptr<MppEntity> mppLevelGetPlayer()
{
    return player;
}

MppTile mppLevelGetTile(int x, int y)
{
    assert(currentLevel != -1);

    if (x >= 0 && y >= 0 && x < MppLevelWidth && y < MppLevelWidth)
    {
        return levels[currentLevel].tiles[x][y];
    }
    else
    {
        return MppTile{};
    }
}