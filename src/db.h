#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DB_NULL_UUID (-1)

typedef struct db_blob db_blob_t;
typedef struct entity entity_t;
typedef struct item item_t;
typedef struct tile tile_t;

void db_blob_ptr(db_blob_t* blob, void* data, uint32_t size);
void db_blob_float(db_blob_t* blob, float* data);
void db_blob_int32_t(db_blob_t* blob, int32_t* data);
void db_blob_uint32_t(db_blob_t* blob, uint32_t* data);
void db_blob_item_t(db_blob_t* blob, item_t* data);

typedef struct
{
    bool new_db;
}
db_header_t;

bool db_init(const char* path);
void db_quit();
void db_set_header(db_header_t* header);
void db_get_header(db_header_t* header);
void db_insert_entity(entity_t* entity, int y);
void db_insert_tile(tile_t* tile, int x, int y, int z);
void db_select_entity(void (*callback)(entity_t* entity, int x));
void db_select_tile(void (*callback)(tile_t* tile, int x, int y, int z));