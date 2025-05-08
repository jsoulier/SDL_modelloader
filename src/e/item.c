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

void e_item_blob(entity_t* entity, blob_t* blob)
{
    e_item_t* item = (e_item_t*) entity;

    blob_item_t(blob, &item->item);
}