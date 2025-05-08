#pragma once

#include "../entity.h"
#include "mob.h"

typedef struct e_player
{
    e_mob_t mob;
}
e_player_t;

void e_player_init(entity_t* entity, void* args);
void e_player_blob(entity_t* entity, blob_t* blob);