#include <sqlite3.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <print>

#include "database.hpp"
#include "entity.hpp"
#include "serializer.hpp"
#include "tile.hpp"
#include "world.hpp"

static constexpr const char* save_file = "lilcraft.sqlite3";

static sqlite3* handle;
static sqlite3_stmt* insert_entity;
static sqlite3_stmt* update_entity;
static sqlite3_stmt* select_entity;

bool Database::init()
{
    if (sqlite3_open(save_file, &handle) != SQLITE_OK)
    {
        std::println("Failed to open database:  {}", sqlite3_errmsg(handle));
        return false;
    }

    const char* sql_string =
        "CREATE TABLE IF NOT EXISTS entities ("
        "    uuid INTEGER PRIMARY KEY,"
        "    level INTEGER NOT NULL,"
        "    type INTEGER NOT NULL,"
        "    blob INTEGER NOT NULL"
        ");";

    if (sqlite3_exec(handle, sql_string, 0, nullptr, nullptr) != SQLITE_OK)
    {
        std::println("Failed to execute SQL: {}", sqlite3_errmsg(handle));
        return false;
    }

    const char* insert_entity_string =
        "INSERT INTO entities (level, type, blob) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(handle, insert_entity_string, -1, &insert_entity, 0) != SQLITE_OK)
    {
        std::println("Failed to prepare insert entity: {}", sqlite3_errmsg(handle));
        return false;
    }

    const char* update_entity_string =
        "UPDATE entities SET level = ?, blob = ? WHERE uuid = ?;";

    if (sqlite3_prepare_v2(handle, update_entity_string, -1, &update_entity, 0) != SQLITE_OK)
    {
        std::println("Failed to prepare update entity: {}", sqlite3_errmsg(handle));
        return false;
    }

    const char* select_entity_string =
        "SELECT uuid, level, type, blob FROM entities;";

    if (sqlite3_prepare_v2(handle, select_entity_string, -1, &select_entity, 0) != SQLITE_OK)
    {
        std::println("Failed to prepare select entity: {}", sqlite3_errmsg(handle));
        return false;
    }

    if (sqlite3_exec(handle, "BEGIN;", 0, 0, 0) != SQLITE_OK)
    {
        std::println("Failed to begin transaction: {}", sqlite3_errmsg(handle));
        return false;
    }

    return true;
}

void Database::shutdown()
{
    commit();

    sqlite3_finalize(insert_entity);
    sqlite3_finalize(update_entity);
    sqlite3_finalize(select_entity);

    sqlite3_free(handle);
}

bool Database::has_save()
{
    return std::filesystem::exists(save_file);
}

void Database::commit()
{
    sqlite3_exec(handle, "COMMIT; BEGIN;", 0, 0, 0);
}

void Database::insert(std::shared_ptr<Entity>& entity, LevelId level)
{
    Serializer writer{};
    entity->serialize(writer);

    const void* data = writer.get_data();
    uint32_t size = writer.get_size();

    if (!entity->get_uuid())
    {
        sqlite3_bind_int(insert_entity, 1, level);
        sqlite3_bind_int(insert_entity, 2, entity->get_type());
        sqlite3_bind_blob(insert_entity, 3, data, size, SQLITE_TRANSIENT);

        if (sqlite3_step(insert_entity) != SQLITE_DONE)
        {
            std::println("Failed to insert entity: {}", sqlite3_errmsg(handle));
        }
        else
        {
            entity->set_uuid(sqlite3_last_insert_rowid(handle));
        }

        sqlite3_reset(insert_entity);
    }
    else
    {
        sqlite3_bind_int(update_entity, 1, level);
        sqlite3_bind_blob(update_entity, 2, data, size, SQLITE_TRANSIENT);
        sqlite3_bind_int(update_entity, 3, entity->get_uuid());

        if (sqlite3_step(update_entity) != SQLITE_DONE)
        {
            std::println("Failed to update entity: {}", sqlite3_errmsg(handle));
            entity->set_uuid(UUID{});
        }

        sqlite3_reset(update_entity);
    }
}

void Database::select(const std::function<void(std::shared_ptr<Entity>&, LevelId)>& callback)
{
    while (sqlite3_step(select_entity) == SQLITE_ROW)
    {
        UUID uuid = sqlite3_column_int(select_entity, 0);

        LevelId level = LevelId(sqlite3_column_int(select_entity, 1));
        EntityType type = EntityType(sqlite3_column_int(select_entity, 2));

        const void* data = sqlite3_column_blob(select_entity, 3);
        uint32_t size = sqlite3_column_bytes(select_entity, 3);

        std::shared_ptr<Entity> entity = Entity::create(type);
        if (!entity)
        {
            std::println("Failed to select entity");
            continue;
        }

        Serializer reader{data, size};
        entity->set_uuid(uuid);
        entity->serialize(reader);

        callback(entity, level);
    }

    sqlite3_reset(select_entity);
}