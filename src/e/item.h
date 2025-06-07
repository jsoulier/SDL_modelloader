#pragma once

#include "../data.h"
#include "../entity.h"

typedef struct e_item
{
    entity_t entity;
    item_t item;
}
e_item_t;

void e_item_init(entity_t* entity, void* args);
void e_item_tick(entity_t* entity, float dt);
void e_item_blob(entity_t* entity, blob_t* blob);