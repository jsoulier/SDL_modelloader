#include "../entity.h"
#include "mob.h"
#include "player.h"

static void init_stub(entity_t* entity, void* args) {}
static void blob_stub(entity_t* entity, db_blob_t* blob) {}

struct
{
    void (*init)(entity_t* entity, void* args);
    void (*blob)(entity_t* entity, db_blob_t* blob);
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

void e_mob_blob(entity_t* entity, db_blob_t* blob)
{

}