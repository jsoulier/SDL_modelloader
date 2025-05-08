#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "data.h"
#include "db.h"
#include "dbg.h"
#include "entity.h"

static SDL_Window* window;
static SDL_GPUDevice* device;

static bool init_window()
{
    SDL_PropertiesID properties = SDL_CreateProperties();
    if (!properties)
    {
        log_release("Failed to create properties: %s", SDL_GetError());
        return false;
    }

    char title[256] = {0};
    if (is_debugging)
    {
        snprintf(title, sizeof(title), "lilcraft [debug]");
    }
    else
    {
        snprintf(title, sizeof(title), "lilcraft");
    }

    SDL_SetStringProperty(properties, "SDL.window.create.title", title);
    SDL_SetNumberProperty(properties, "SDL.window.create.width", 960);
    SDL_SetNumberProperty(properties, "SDL.window.create.height", 720);
    SDL_SetBooleanProperty(properties, "SDL.window.create.resizable", true);

    window = SDL_CreateWindowWithProperties(properties);
    if (!window)
    {
        log_release("Failed to create window: %s", SDL_GetError());
        return false;
    }

    SDL_DestroyProperties(properties);

    return true;
}

static bool init_device()
{
    return true;
}

int main(int argc, char** argv)
{
    SDL_SetAppMetadata("lilcraft", NULL, NULL);

    log_init();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        log_release("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!TTF_Init())
    {
        log_release("Failed to initialize SDL ttf: %s", SDL_GetError());
        return 1;
    }

    if (!init_window())
    {
        log_release("Failed to initialize window");
        return 1;
    }

    if (!init_device())
    {
        log_release("Failed to initialize device");
        return 1;
    }

    if (!db_init("lilcraft.sqlite3"))
    {
        log_release("Failed to initialize database");
        log_release("Playing without saves enabled");
    }

    db_header_t db_header;
    db_get_header(&db_header);

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            }
        }
        if (!running)
        {
            break;
        }
    }

    SDL_HideWindow(window);

    db_quit();

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);

    log_quit();

    TTF_Quit();
    SDL_Quit();

    return 0;
}