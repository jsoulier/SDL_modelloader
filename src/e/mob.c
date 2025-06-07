#include <float.h>
#include <math.h>

#include "../data.h"
#include "../entity.h"
#include "mob.h"
#include "player.h"

#define speed 200.0f

struct
{
    void (*init)(entity_t* entity, void* args);
    void (*blob)(entity_t* entity, blob_t* blob);
    void (*move)(entity_t* entity, float* dx, float* dz);
}
static const vtable[entity_type_count] =
{
    [entity_type_player] =
    {
        .init = e_player_init,
        .blob = e_player_blob,
        .move = e_player_move,
    },
};

void e_mob_init(entity_t* entity, void* args)
{

}

void e_mob_tick(entity_t* entity, float dt)
{
    e_mob_t* mob = (e_mob_t*) entity;

    float dx = 0.0f;
    float dz = 0.0f;

    vtable[entity->type].move(entity, &dx, &dz);

    float length = sqrtf(dx * dx + dz * dz);
    if (length < FLT_EPSILON)
    {
        return;
    }

    dx /= length;
    dz /= length;

    entity->x += dx * speed * dt;
    entity->z += dz * speed * dt;

    entity->rotation = atan2(dz, dx);
}

void e_mob_blob(entity_t* entity, blob_t* blob)
{

}

mesh_type_t e_mob_get_mesh_type(const entity_t* entity)
{
    e_mob_t* mob = (e_mob_t*) entity;

    return /* TODO: */ mesh_type_tree_00;
}
