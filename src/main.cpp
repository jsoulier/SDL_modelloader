#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <print>

int main(int argc, char** argv)
{
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("lilcraft", nullptr, nullptr);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::println("Failed to initialize SDL: {}", SDL_GetError());
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
    }

    SDL_Quit();

    return 0;
}