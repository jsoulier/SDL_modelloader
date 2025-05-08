#include <sqlite3.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "db.h"
#include "dbg.h"
#include "entity.h"

typedef struct blob
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
blob_t;

static void blob_init(blob_t* blob, bool is_writer)
{
    assert(blob);

    blob->reader.size = 0;
    blob->writer.size = 0;

    blob->is_writer = is_writer;
    blob->failed = false;
}

static void blob_free(blob_t* blob)
{
    assert(blob);

    if (blob->writer.data)
    {
        free(blob->writer.data);
        blob->writer.data = NULL;
    }
}

bool blob_ptr(blob_t* blob, void* data, uint32_t size)
{
    assert(blob);
    assert(data);
    assert(size);

    if (blob->failed)
    {
        return false;
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
                log_release("Failed to reallocate blob");
                return false;
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
            log_release("Exceeded blob reader capacity");
            return false;
        }

        memcpy(data, blob->reader.data + blob->reader.size, size);
        blob->reader.size += size;
    }

    blob->failed = false;

    return true;
}

bool blob_float(blob_t* blob, float* data)
{
    return blob_ptr(blob, data, sizeof(*data));
}

bool blob_int32_t(blob_t* blob, int32_t* data)
{
    return blob_ptr(blob, data, sizeof(*data));
}

bool blob_uint32_t(blob_t* blob, uint32_t* data)
{
    return blob_ptr(blob, data, sizeof(*data));
}

bool blob_item_t(blob_t* blob, item_t* data)
{
    return blob_ptr(blob, data, sizeof(*data));
}

static sqlite3* handle;

static sqlite3_stmt* set_header;
static sqlite3_stmt* get_header;

static sqlite3_stmt* insert_entity;
static sqlite3_stmt* update_entity;
static sqlite3_stmt* select_entity;
static sqlite3_stmt* delete_entity;

static sqlite3_stmt* insert_tile;
static sqlite3_stmt* select_tile;

static blob_t blob;

static bool is_open;

bool db_init(const char* path)
{
    assert(path);

    if (sqlite3_open(path, &handle) != SQLITE_OK)
    {
        log_release("Failed to open database: %s, %s", path, sqlite3_errmsg(handle));
        return false;
    }

    const char* create_tables_sql =
        "CREATE TABLE IF NOT EXISTS header ("
        "    id INTEGER PRIMARY KEY,"
        "    new_db INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS entity ("
        "    uuid INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    type INTEGER NOT NULL,"
        "    y INTEGER NOT NULL,"
        "    data BLOB NOT NULL"
        ");";

    if (sqlite3_exec(handle, create_tables_sql, 0, NULL, NULL) != SQLITE_OK)
    {
        log_release("Failed to create tables: %s, %s", path, sqlite3_errmsg(handle));
        return false;
    }

    const char* set_header_sql =
        "INSERT OR REPLACE INTO header (id, new_db) VALUES (?, ?);";

    if (sqlite3_prepare_v2(handle, set_header_sql, -1, &set_header, 0) != SQLITE_OK)
    {
        log_release("Failed to prepare set header: %s", sqlite3_errmsg(handle));
        return false;
    }

    const char* get_header_sql =
        "SELECT new_db FROM header WHERE id = ?;";

    if (sqlite3_prepare_v2(handle, get_header_sql, -1, &get_header, 0) != SQLITE_OK)
    {
        log_release("Failed to prepare get header: %s", sqlite3_errmsg(handle));
        return false;
    }

    if (sqlite3_exec(handle, "BEGIN;", 0, 0, 0) != SQLITE_OK)
    {
        log_release("Failed to begin transaction: %s", sqlite3_errmsg(handle));
        return false;
    }

    is_open = true;

    return true;
}

void db_quit()
{
    sqlite3_exec(handle, "COMMIT;", 0, 0, 0);

    sqlite3_finalize(get_header);
    sqlite3_finalize(set_header);

    sqlite3_finalize(insert_entity);
    sqlite3_finalize(update_entity);
    sqlite3_finalize(select_entity);
    sqlite3_finalize(delete_entity);

    sqlite3_finalize(insert_tile);
    sqlite3_finalize(select_tile);

    sqlite3_free(handle);

    is_open = false;
}

void db_set_header(db_header_t* header)
{
    assert(header);

    sqlite3_bind_int(set_header, 1, 0);
    sqlite3_bind_int(set_header, 2, header->new_db);

    if (sqlite3_step(set_header) != SQLITE_DONE)
    {
        log_release("Failed to set header: %s", sqlite3_errmsg(handle));
    }

    sqlite3_reset(set_header);
}

void db_get_header(db_header_t* header)
{
    assert(header);

    sqlite3_bind_int(get_header, 1, 0);
    if (sqlite3_step(get_header) == SQLITE_ROW)
    {
        header->new_db = sqlite3_column_int(get_header, 0);
    }
    else
    {
        header->new_db = true;
    }
}

void db_insert_entity(entity_t* entity, int y)
{
    assert(entity);

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

void db_select_entity(void (*callback)(entity_t* entity, int x))
{
    assert(callback);

    if (!is_open)
    {
        return;
    }
}

void db_select_tile(void (*callback)(tile_t* tile, int x, int y, int z))
{
    assert(callback);

    if (!is_open)
    {
        return;
    }
}

void db_delete_entity(entity_t* entity)
{
    assert(entity);

    if (!is_open)
    {
        return;
    }
}