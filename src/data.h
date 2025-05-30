#pragma once

typedef enum tile_type
{
    tile_type_count,
}
tile_type_t;

typedef struct tile
{
    tile_type_t type;
}
tile_t;

typedef enum item_type
{
    item_type_count,
}
item_type_t;

typedef struct item
{
    item_type_t item;
}
item_t;