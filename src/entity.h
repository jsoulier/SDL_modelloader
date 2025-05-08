#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "data.h"

typedef struct blob blob_t;

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

entity_t* entity_create(entity_type_t type, void* args);
void entity_free(entity_t* entity);
void entity_blob(entity_t* entity, blob_t* blob);