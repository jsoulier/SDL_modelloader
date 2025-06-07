#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "data.h"
#include "db.h"
#include "e/item.h"
#include "e/mob.h"
#include "e/player.h"
#include "entity.h"
#include "util.h"

struct
{
    void (*init)(entity_t* entity, void* args);
    void (*tick)(entity_t* entity, float dt);
    void (*blob)(entity_t* entity, blob_t* blob);

    mesh_type_t (*get_mesh_type)(const entity_t* entity);

    uint32_t size;
}
static const vtable[entity_type_count] =
{
    [entity_type_player] =
    {
        .init = e_mob_init,
        .tick = e_mob_tick,
        .blob = e_mob_blob,
        .get_mesh_type = e_mob_get_mesh_type,
        .size = sizeof(e_player_t),
    },
    [entity_type_item] =
    {
        .init = e_item_init,
        .tick = e_item_tick,
        .blob = e_item_blob,
        .get_mesh_type = e_item_get_mesh_type,
        .size = sizeof(e_item_t),
    },
};

entity_t* entity_create(entity_type_t type, void* args)
{
    /* TODO: better allocation strategy */

    assert_debug(type >= 0);
    assert_debug(type < entity_type_count);

    entity_t* entity = malloc(vtable[type].size);
    if (!entity)
    {
        log_release("Failed to allocate entity");
        return NULL;
    }

    entity->next = NULL;

    entity->uuid = db_null_uuid;
    entity->type = type;

    entity->x = 0.0f;
    entity->y = 0.0f;
    entity->z = 0.0f;
    entity->rotation = 0.0f;
    entity->radius = 16.0f;

    entity->alive = true;

    vtable[type].init(entity, args);

    return entity;
}

void entity_free(entity_t* entity)
{
    assert_debug(entity);

    free(entity);
}

void entity_tick(entity_t* entity, float dt)
{
    assert_debug(entity);

    vtable[entity->type].tick(entity, dt);
}

void entity_blob(entity_t* entity, blob_t* blob)
{
    assert_debug(entity);
    assert_debug(blob);

    blob_float(blob, &entity->x);
    blob_float(blob, &entity->y);
    blob_float(blob, &entity->z);
    blob_float(blob, &entity->rotation);

    vtable[entity->type].blob(entity, blob);
}

mesh_type_t entity_get_mesh_type(const entity_t* entity)
{
    assert_debug(entity);

    return vtable[entity->type].get_mesh_type(entity);
}

transform_t entity_get_transform(const entity_t* entity)
{
    assert_debug(entity);

    transform_t transform;

    transform.position.x = entity->x;
    transform.position.y = entity->y;
    transform.position.z = entity->z;
    transform.rotation = entity->rotation;

    return transform;
}

aabb_t entity_get_aabb(const entity_t* entity)
{
    assert_debug(entity);

    aabb_t aabb;

    aabb.min.x = entity->x - entity->radius;
    aabb.min.z = entity->z - entity->radius;
    aabb.max.x = entity->x + entity->radius;
    aabb.max.z = entity->z + entity->radius;

    return aabb;
}
