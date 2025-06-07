#include "../entity.h"
#include "mob.h"
#include "player.h"

struct
{
    void (*init)(entity_t* entity, void* args);
    void (*blob)(entity_t* entity, blob_t* blob);
}
static const vtable[entity_type_count] =
{
    [entity_type_player] =
    {
        .init = e_player_init,
        .blob = e_player_blob,
    },
};

void e_mob_init(entity_t* entity, void* args)
{

}

void e_mob_tick(entity_t* entity, float dt)
{

}

void e_mob_blob(entity_t* entity, blob_t* blob)
{

}