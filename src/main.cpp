#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <print>

#include "components.hpp"
#include "graphics.hpp"

#ifndef NDEBUG
static constexpr bool debugging = true;
#else
static constexpr bool debugging = false;
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

    if (!graphics::init(debugging))
    {
        std::println("Failed to initialize graphics");
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

            if (debugging)
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
            }
        }

        graphics::draw();
    }

    graphics::quit();

    SDL_Quit();

    return 0;
}