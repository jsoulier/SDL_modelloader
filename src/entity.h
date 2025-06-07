#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "data.h"
#include "math_ex.h"

typedef struct blob blob_t;
typedef struct entity entity_t;

typedef enum entity_type
{
    entity_type_player,
    entity_type_item,

    entity_type_count,
}
entity_type_t;

typedef struct entity
{
    entity_t* next;

    int64_t uuid;
    entity_type_t type;

    bool alive;

    float x;
    float y;
    float z;
    float rotation;
    float radius;
}
entity_t;

entity_t* entity_create(entity_type_t type, void* args);
void entity_free(entity_t* entity);
void entity_tick(entity_t* entity, float dt);
void entity_blob(entity_t* entity, blob_t* blob);
mesh_type_t entity_get_mesh_type(const entity_t* entity);
transform_t entity_get_transform(const entity_t* entity);
aabb_t entity_get_aabb(const entity_t* entity);
