#include "../data.h"
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

mesh_type_t e_mob_get_mesh_type(const entity_t* entity)
{
    e_mob_t* mob = (e_mob_t*) entity;

    return /* TODO: */ mesh_type_tree_00;
}
