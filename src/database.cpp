#include <sqlite3.h>

#include "database.hpp"
#include "entity.hpp"
#include "serializer.hpp"
#include "tile.hpp"

static sqlite3* handle;

bool Database::init()
{
    return true;
}

void Database::shutdown()
{

}