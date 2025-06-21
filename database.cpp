#include <SDL3/SDL.h>

#include <cstdint>
#include <functional>
#include <memory>

#include "database.hpp"
#include "entity.hpp"
#include "sqlite3.h"

static constexpr const char* SaveFile = "minicraft++.sqlite3";

static sqlite3* handle;
static sqlite3_stmt* getTimeStmt;
static sqlite3_stmt* setTimeStmt;
static sqlite3_stmt* insertEntityStmt;
static sqlite3_stmt* updateEntityStmt;
static sqlite3_stmt* selectEntityStmt;

static bool createTables()
{
    const char* tablesSQL =
        "CREATE TABLE IF NOT EXISTS header ("
        "    id INTEGER PRIMARY KEY,"
        "    time INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS entity ("
        "    type INTEGER NOT NULL,"
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    x FLOAT NOT NULL,"
        "    y FLOAT NOT NULL,"
        "    level INTEGER NOT NULL,"
        "    data BLOB"
        ");";

    if (sqlite3_exec(handle, tablesSQL, 0, nullptr, nullptr) != SQLITE_OK)
    {
        SDL_Log("Failed to execute SQL: %s", sqlite3_errmsg(handle));
        return false;
    }

    return true;
}

static bool createHeaderStatements()
{
    const char* setTimeSQL =
        "INSERT OR REPLACE INTO header (id, time) VALUES (?, ?);";

    if (sqlite3_prepare_v2(handle, setTimeSQL, -1, &setTimeStmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare set time: %s", sqlite3_errmsg(handle));
        return false;
    }

    const char* getTimeSQL =
        "SELECT time FROM header WHERE id = ?;";

    if (sqlite3_prepare_v2(handle, getTimeSQL, -1, &getTimeStmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare get time: %s", sqlite3_errmsg(handle));
        return false;
    }

    return true;
}

static bool createEntityStatements()
{
    const char* insertEntitySQL =
        "INSERT INTO entity (type, x, y, level, data) VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(handle, insertEntitySQL, -1, &insertEntityStmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare insert entity: %s", sqlite3_errmsg(handle));
        return false;
    }

    const char* updateEntitySQL =
        "UPDATE entity SET x = ?, y = ?, level = ?, data = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(handle, updateEntitySQL, -1, &updateEntityStmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare update entity: %s", sqlite3_errmsg(handle));
        return false;
    }

    const char* selectEntitySQL =
        "SELECT * FROM entity;";

    if (sqlite3_prepare_v2(handle, selectEntitySQL, -1, &selectEntityStmt, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to prepare select entity: %s", sqlite3_errmsg(handle));
        return false;
    }

    return true;
}

bool mppDatabaseInit()
{
    bool exists = SDL_GetPathInfo(SaveFile, nullptr);

    if (sqlite3_open(SaveFile, &handle) != SQLITE_OK)
    {
        SDL_Log("Failed to open database: %s, %s", SaveFile, sqlite3_errmsg(handle));
        return false;
    }

    if (!createTables())
    {
        SDL_Log("Failed to create tables");
        return false;
    }

    if (!createHeaderStatements())
    {
        SDL_Log("Failed to create header statements");
        return false;
    }

    if (!createEntityStatements())
    {
        SDL_Log("Failed to create entity statements");
        return false;
    }

    sqlite3_exec(handle, "BEGIN;", 0, 0, 0);

    return exists;
}

void mppDatabaseQuit()
{
    if (!handle)
    {
        return;
    }

    sqlite3_exec(handle, "COMMIT;", 0, 0, 0);

    sqlite3_finalize(setTimeStmt);
    sqlite3_finalize(getTimeStmt);

    sqlite3_finalize(insertEntityStmt);
    sqlite3_finalize(updateEntityStmt);
    sqlite3_finalize(selectEntityStmt);

    sqlite3_close(handle);
}

void mppDatabaseCommit()
{
    if (!handle)
    {
        return;
    }

    sqlite3_exec(handle, "COMMIT; BEGIN;", 0, 0, 0);
}

void mppDatabaseSetTime(int64_t time)
{
    if (!handle)
    {
        return;
    }

    sqlite3_bind_int(setTimeStmt, 1, 0);
    sqlite3_bind_int64(setTimeStmt, 2, time);

    if (sqlite3_step(setTimeStmt) != SQLITE_DONE)
    {
        SDL_Log("Failed to set time: %s", sqlite3_errmsg(handle));
    }

    sqlite3_reset(setTimeStmt);
}

int64_t mppDatabaseGetTime()
{
    if (!handle)
    {
        return 0;
    }

    sqlite3_bind_int(getTimeStmt, 1, 0);

    if (sqlite3_step(getTimeStmt) != SQLITE_ROW)
    {
        return 0;
    }

    int64_t time = sqlite3_column_int64(getTimeStmt, 0);

    sqlite3_reset(getTimeStmt);

    return time;
}

static void insertEntity(std::shared_ptr<MppEntity>& entity)
{
    sqlite3_bind_int(insertEntityStmt, 1, entity->getType());
    sqlite3_bind_double(insertEntityStmt, 2, entity->getX());
    sqlite3_bind_double(insertEntityStmt, 3, entity->getY());
    sqlite3_bind_double(insertEntityStmt, 4, entity->getLevel());
    sqlite3_bind_null(insertEntityStmt, 5);

    /* TODO: blob */

    if (sqlite3_step(insertEntityStmt) == SQLITE_DONE)
    {
        entity->setId(sqlite3_last_insert_rowid(handle));
    }
    else
    {
        SDL_Log("Failed to insert entity: %s", sqlite3_errmsg(handle));
    }

    sqlite3_reset(insertEntityStmt);
}

static void updateEntity(std::shared_ptr<MppEntity>& entity)
{
    sqlite3_bind_double(updateEntityStmt, 1, entity->getX());
    sqlite3_bind_double(updateEntityStmt, 2, entity->getY());
    sqlite3_bind_int(updateEntityStmt, 3, entity->getLevel());
    sqlite3_bind_null(updateEntityStmt, 4);
    sqlite3_bind_int64(updateEntityStmt, 5, entity->getId());

    /* TODO: blob */

    if (sqlite3_step(updateEntityStmt) != SQLITE_DONE)
    {
        SDL_Log("Failed to update entity: %s", sqlite3_errmsg(handle));
        entity->setId(-1);
    }

    sqlite3_reset(updateEntityStmt);
}

void mppDatabaseInsert(std::shared_ptr<MppEntity>& entity)
{
    if (!handle)
    {
        return;
    }

    if (entity->getId() == -1)
    {
        insertEntity(entity);
    }
    else
    {
        updateEntity(entity);
    }
}

static void selectEntity(const std::function<void(std::shared_ptr<MppEntity>&)>& function)
{
    int type = sqlite3_column_int(selectEntityStmt, 0);

    std::shared_ptr<MppEntity> entity = mppEntityCreate(type);
    if (!entity)
    {
        SDL_Log("Failed to select entity");
        return;
    }

    entity->setId(sqlite3_column_int64(selectEntityStmt, 1));
    entity->setX(sqlite3_column_double(selectEntityStmt, 2));
    entity->setY(sqlite3_column_double(selectEntityStmt, 3));
    entity->setLevel(sqlite3_column_int(selectEntityStmt, 4));

    /* TODO: blob */

    function(entity);
}

void mppDatabaseSelect(const std::function<void(std::shared_ptr<MppEntity>&)>& function)
{
    if (!handle)
    {
        return;
    }

    while (sqlite3_step(selectEntityStmt) == SQLITE_ROW)
    {
        selectEntity(function);
    }

    sqlite3_reset(selectEntityStmt);
}

void mppDatabaseInsert(const MppTile& tile, int x, int y, int level)
{
    if (!handle)
    {
        return;
    }

    /* TODO: */
}

void mppDatabaseSelect(const std::function<void(MppTile&, int, int, int)>& function)
{
    if (!handle)
    {
        return;
    }

    /* TODO: */
}