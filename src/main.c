#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdbool.h>
#include <stddef.h>

#include "data.h"
#include "db.h"
#include "mesh.h"
#include "util.h"

static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_IOStream* iostream;

static void log_callback(void* data, int category, SDL_LogPriority priority, const char* string)
{
    if (!iostream)
    {
        iostream = SDL_IOFromFile("lilcraft.log", "w");
    }

    if (is_debugging)
    {
        SDL_GetDefaultLogOutputFunction()(data, category, priority, string);
    }

    if (iostream)
    {
        SDL_IOprintf(iostream, "%s\n", string);
    }
}

static bool init_window()
{
    SDL_PropertiesID properties = SDL_CreateProperties();

    SDL_SetStringProperty(properties, "SDL.window.create.title", "lilcraft");
    SDL_SetNumberProperty(properties, "SDL.window.create.width", 960);
    SDL_SetNumberProperty(properties, "SDL.window.create.height", 720);
    SDL_SetBooleanProperty(properties, "SDL.window.create.resizable", true);

    window = SDL_CreateWindowWithProperties(properties);
    if (!window)
    {
        error("Failed to create window: %s", SDL_GetError());
        return false;
    }

    SDL_DestroyProperties(properties);

    return true;
}

static bool init_device()
{
    SDL_PropertiesID properties = SDL_CreateProperties();

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.spirv", true);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.dxil", true);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.msl", true);

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.debugmode", is_debugging);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.verbose", is_debugging);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.preferlowpower", true);

    device = SDL_CreateGPUDeviceWithProperties(properties);
    if (!device)
    {
        error("Failed to create device: %s", SDL_GetError());
        return false;
    }

    SDL_DestroyProperties(properties);

    return true;
}

int main(int argc, char** argv)
{
    SDL_SetAppMetadata("lilcraft", NULL, NULL);

    if (is_debugging)
    {
        SDL_SetLogOutputFunction(log_callback, NULL);
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        error("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!init_window())
    {
        error("Failed to initialize window");
        return 1;
    }

    if (!init_device())
    {
        error("Failed to initialize device");
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        error("Failed to create swapchain");
        return 1;
    }

    if (!db_init("lilcraft.sqlite3"))
    {
        error("Failed to initialize database");
        warning("Playing without saves enabled");
    }

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
    }

    db_quit();

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    SDL_CloseIO(iostream);

    return 0;
}