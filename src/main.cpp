#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdint>
#include <print>

#include "math.hpp"
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

    if (!init_renderer(debug))
    {
        std::println("Failed to initialize renderer");
        return 1;
    }

    bool running = true;
    uint64_t t1 = SDL_GetPerformanceCounter();
    uint64_t t2 = 0;
    while (running)
    {
        t2 = t1;
        t1 = SDL_GetPerformanceCounter();
        const float frequency = SDL_GetPerformanceFrequency();
        const float dt = (t1 - t2) / frequency;

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

        wait_for_renderer();
        move_renderer(Transform{});
        render_model(model_grass, Transform{});
        render_frame(dt);
    }

    shutdown_renderer();
    shutdown_world();
    shutdown_database();

    SDL_Quit();

    return 0;
}