#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "db.h"
#include "util.h"

int main(int argc, char** argv)
{
    if (!db_init("lilcraft.sqlite3"))
    {
        error("Failed to initialize database");
        warning("Playing without saves enabled");
    }

    db_quit();

    return 0;
}