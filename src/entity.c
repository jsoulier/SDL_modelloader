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

    uint32_t size;
}
static const vtable[entity_type_count] =
{
    [entity_type_player] =
    {
        .init = e_mob_init,
        .tick = e_mob_tick,
        .blob = e_mob_blob,
        .size = sizeof(e_player_t),
    },
    [entity_type_item] =
    {
        .init = e_item_init,
        .tick = e_item_tick,
        .blob = e_item_blob,
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

    entity->transform.position.x = 0.0f;
    entity->transform.position.y = 0.0f;
    entity->transform.position.z = 0.0f;
    entity->transform.rotation = 0.0f;

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

    blob_transform_t(blob, &entity->transform);

    vtable[entity->type].blob(entity, blob);
}