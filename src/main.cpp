#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>
#include <cstdint>
#include <print>

#include "database.hpp"
#include "entity.hpp"
#include "math.hpp"
#include "noise.hpp"
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

    bool has_save = Database::has_save();

    if (!Database::init())
    {
        std::println("Failed to initialize database");
        return 1;
    }

    if (!World::init())
    {
        std::println("Failed to initialize world");
        return 1;
    }

    if (!Renderer::init(debug))
    {
        std::println("Failed to initialize renderer");
        return 1;
    }

    if (!has_save && !Noise::generate())
    {
        std::println("Failed to generate");
        return 1;
    }

    std::shared_ptr<Entity> player = World::get_player();
    if (!player)
    {
        std::println("Failed to get player");
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

        Renderer::wait();

        Renderer::move(player->get_transform());

        World::move();
        World::update(dt);
        World::draw();

        Renderer::submit(dt);
    }

    World::commit();

    Renderer::shutdown();
    World::shutdown();
    Database::shutdown();

    SDL_Quit();

    return 0;
}