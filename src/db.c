#include <sqlite3.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "entity.h"
#include "util.h"

typedef struct db_blob
{
    struct
    {
        uint32_t size;
        uint32_t capacity;
        uint8_t* data;
    }
    reader, writer;

    bool is_writer;
    bool failed;
}
db_blob_t;

static void set_blob(db_blob_t* blob, bool is_writer)
{
    blob->is_writer = is_writer;
    blob->failed = false;

    blob->reader.size = 0;
    blob->writer.size = 0;
}

void db_blob_ptr(db_blob_t* blob, void* data, uint32_t size)
{
    if (blob->failed)
    {
        return;
    }

    blob->failed = true;

    if (blob->is_writer)
    {
        if (blob->writer.size + size > blob->writer.capacity)
        {
            uint32_t new_capacity = (blob->writer.size + size) * 2;

            blob->writer.data = realloc(blob->writer.data, new_capacity);
            if (!blob->writer.data)
            {
                error("Failed to reallocate blob");
                return;
            }

            blob->writer.capacity = new_capacity;
        }

        memcpy(blob->writer.data + blob->writer.size, data, size);
        blob->writer.size += size;
    }
    else
    {
        if (blob->reader.capacity < blob->reader.size + size)
        {
            error("Exceeded blob reader capacity");
            return;
        }

        memcpy(data, blob->reader.data + blob->reader.size, size);
        blob->reader.size += size;
    }

    blob->failed = false;
}

void db_blob_float(db_blob_t* blob, float* data)
{
    db_blob_ptr(blob, data, sizeof(*data));
}

void db_blob_int32_t(db_blob_t* blob, int32_t* data)
{
    db_blob_ptr(blob, data, sizeof(*data));
}

void db_blob_uint32_t(db_blob_t* blob, uint32_t* data)
{
    db_blob_ptr(blob, data, sizeof(*data));
}

static sqlite3* handle;

static sqlite3_stmt* insert_entity;
static sqlite3_stmt* update_entity;

static bool is_open;

static db_blob_t blob;

bool db_init(const char* path)
{
    assert(path);

    if (sqlite3_open(path, &handle) != SQLITE_OK)
    {
        error("Failed to open database: %s, %s", path, sqlite3_errmsg(handle));
        return false;
    }

    is_open = true;

    return true;
}

void db_quit()
{
    sqlite3_finalize(insert_entity);
    sqlite3_finalize(update_entity);

    sqlite3_free(handle);

    is_open = false;
}

void db_insert_entity(entity_t* entity, int y)
{
    assert(entity);

    set_blob(&blob, true);

    if (!is_open)
    {
        return;
    }
}

void db_insert_tile(tile_t* tile, int x, int y, int z)
{
    assert(tile);

    if (!is_open)
    {
        return;
    }
}

void db_select_entity(void (*callback)(entity_t*, int))
{
    assert(callback);

    if (!is_open)
    {
        return;
    }

    while (1 /* todo */)
    {
        set_blob(&blob, false);
    }
}

void db_select_tile(void (*callback)(tile_t*, int, int, int))
{
    assert(callback);

    if (!is_open)
    {
        return;
    }
}