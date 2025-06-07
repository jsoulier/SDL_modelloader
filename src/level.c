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

void level_each_entity(void (*callback)(const entity_t* entity, void* data), const aabb_t* aabb, void *data)
{
    assert_debug(callback);
    assert_debug(aabb);

    entity_t* entity = levels[level].entities;

    while (entity)
    {
        aabb_t aabb2 = entity_get_aabb(entity);
        
        if (aabb_test(aabb, &aabb2))
        {
            callback(entity, data);
        }
    
        entity = entity->next;
    }
}

void level_each_tile(void (*callback)(const tile_t* tile, int x, int z, void* data), const aabb_t* aabb, void *data)
{
    assert_debug(callback);
    assert_debug(aabb);
}