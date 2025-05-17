#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <print>

#include "database.hpp"
#include "renderer.hpp"
#include "world.hpp"

#ifndef NDEBUG
static constexpr bool debug = true;
#else
static constexpr bool debug = false;
#endif

int main(int argc, char** argv)
{
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("lilcraft", nullptr, nullptr);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::println("Failed to initialize SDL: {}", SDL_GetError());
        return 1;
    }

    if (!init_renderer(debug))
    {
        std::println("Failed to initialize renderer");
        return 1;
    }

    if (!init_database())
    {
        std::println("Failed to initialize database");
        return 1;
    }

    if (!init_world())
    {
        std::println("Failed to initialize world");
        return 1;
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

        render();
    }

    shutdown_world();
    shutdown_database();
    shutdown_renderer();

    SDL_Quit();

    return 0;
}