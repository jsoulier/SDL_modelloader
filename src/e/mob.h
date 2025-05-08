#pragma once

#include "../entity.h"

typedef struct e_mob
{
    entity_t entity;
}
e_mob_t;

void e_mob_init(entity_t* entity, void* args);
void e_mob_blob(entity_t* entity, blob_t* blob);