#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "database.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "renderer.hpp"

static uint64_t databaseTime;
static uint64_t currTime;
static uint64_t t1;
static uint64_t t2;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    SDL_SetAppMetadata("Minicraft Plus Plus", nullptr, nullptr);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

    if (!mppRendererInit())
    {
        SDL_Log("Failed to initialize renderer");
        return SDL_APP_FAILURE;
    }

    if (!mppLevelInit(mppDatabaseInit()))
    {
        SDL_Log("Failed to initialize level");
        return SDL_APP_FAILURE;
    }

    databaseTime = mppDatabaseGetTime();
    t2 = SDL_GetTicks();
    t1 = t2;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    mppLevelQuit();
    mppDatabaseQuit();
    mppRendererQuit();
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    t2 = SDL_GetTicks();
    uint64_t dt = t2 - t1;
    t1 = t2;
    currTime = databaseTime + t2;

    mppLevelUpdate(dt, currTime);
    mppRendererClear();
    mppLevelRender();
    mppRendererPresent();

    /* TODO: commit cooldown */

    mppLevelCommit();
    mppDatabaseSetTime(currTime);
    mppDatabaseCommit();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_FAILURE;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_MOTION:
        break;
    }

    return SDL_APP_CONTINUE;
}