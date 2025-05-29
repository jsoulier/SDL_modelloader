#pragma once

#include <stdint.h>

typedef struct db_blob db_blob_t;

typedef enum entity_type
{
    entity_type_player,
    entity_type_item,

    entity_type_count,
}
entity_type_t;

typedef struct entity
{
    int64_t uuid;
    entity_type_t type;

    float x;
    float y;
    float z;
    float rotation;
}
entity_t;

entity_t* entity_create_args(entity_type_t type, void* args);
entity_t* entity_create(entity_type_t type);
void entity_free(entity_t* entity);

void entity_blob(entity_t* entity, db_blob_t* blob);