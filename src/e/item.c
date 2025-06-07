#include "../db.h"
#include "../entity.h"
#include "item.h"

void e_item_init(entity_t* entity, void* args)
{
    e_item_t* item = (e_item_t*) entity;

    if (args)
    {
        item->item = *(item_t*) args;
    }
    else
    {
        /* TODO: */
    }
}

void e_item_tick(entity_t* entity, float dt)
{

}

void e_item_blob(entity_t* entity, blob_t* blob)
{
    e_item_t* item = (e_item_t*) entity;

    blob_item_t(blob, &item->item);
}

mesh_type_t e_item_get_mesh_type(const entity_t* entity)
{
    e_item_t* item = (e_item_t*) entity;

    return item_get_mesh_type(&item->item);
}
