#include <stdbool.h>
#include <stddef.h>

#include "data.h"
#include "db.h"
#include "entity.h"
#include "level.h"
#include "math_ex.h"
#include "util.h"

struct
{
    tile_t tiles[level_width][level_width];
    entity_t* entities;
}
levels[level_depth];

static int level;

bool level_init(bool new_level)
{
    return true;
}

void level_quit()
{
    /* TODO: */
}

void level_tick(float dt, const aabb_t* aabb)
{
    entity_t* entities = NULL;

    entity_t* entity = levels[level].entities;

    while (entity)
    {
        if (entity->alive)
        {
            entity_tick(entity, dt);
        }

        if (entity->alive)
        {
            entity->next = entities;
            entities = entity;
            entity = entity->next;
        }
        else
        {
            entity_t* next = entity->next;
            entity_free(entity);
            entity = next;
        }
    }

    levels[level].entities = entities;
}

void level_each_entity(void (*callback)(const entity_t* entity), const aabb_t* aabb)
{
    assert_debug(callback);
    assert_debug(aabb);
}

void level_each_tile(void (*callback)(const tile_t* tile, int x, int z), const aabb_t* aabb)
{
    assert_debug(callback);
    assert_debug(aabb);
}