#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "db.h"
#include "e/item.h"
#include "e/mob.h"
#include "e/player.h"
#include "entity.h"
#include "util.h"

struct
{
    void (*init)(entity_t* entity, void* args);
    void (*blob)(entity_t* entity, db_blob_t* blob);

    uint32_t size;
}
static const vtable[entity_type_count] =
{
    [entity_type_player] =
    {
        .init = e_mob_init,
        .blob = e_mob_blob,
        .size = sizeof(e_player_t),
    },
    [entity_type_item] =
    {
        .init = e_item_init,
        .blob = e_item_blob,
        .size = sizeof(e_item_t),
    },
};

entity_t* entity_create_args(entity_type_t type, void* args)
{
    assert(type >= 0);
    assert(type < entity_type_count);

    entity_t* entity = malloc(vtable[type].size);
    if (!entity)
    {
        error("Failed to allocate entity");
        return NULL;
    }

    entity->uuid = DB_NULL_UUID;
    entity->type = type;

    entity->x = 0.0f;
    entity->y = 0.0f;
    entity->z = 0.0f;
    entity->rotation = 0.0f;

    assert(vtable[type].init);
    vtable[type].init(entity, args);

    return entity;
}

entity_t* entity_create(entity_type_t type)
{
    return entity_create_args(type, NULL);
}

void entity_free(entity_t* entity)
{
    free(entity);
}

void entity_blob(entity_t* entity, db_blob_t* blob)
{
    assert(entity);
    assert(blob);

    db_blob_float(blob, &entity->x);
    db_blob_float(blob, &entity->y);
    db_blob_float(blob, &entity->z);

    assert(vtable[entity->type].blob);
    vtable[entity->type].blob(entity, blob);
}