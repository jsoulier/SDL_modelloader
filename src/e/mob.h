#pragma once

#include "../data.h"
#include "../entity.h"

typedef struct e_mob
{
    entity_t entity;
}
e_mob_t;

void e_mob_init(entity_t* entity, void* args);
void e_mob_tick(entity_t* entity, float dt);
void e_mob_blob(entity_t* entity, blob_t* blob);
mesh_type_t e_mob_get_mesh_type(const entity_t* entity);
