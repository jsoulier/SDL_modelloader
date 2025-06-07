#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stddef.h>

#include "../entity.h"
#include "mob.h"
#include "player.h"

void e_player_init(entity_t* entity, void* args)
{

}

void e_player_blob(entity_t* entity, blob_t* blob)
{

}

void e_player_move(const entity_t* entity, float* dx, float* dz)
{
    const bool* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W])
    {
        (*dz)++;
    }
    if (keys[SDL_SCANCODE_A])
    {
        (*dx)--;
    }
    if (keys[SDL_SCANCODE_S])
    {
        (*dz)--;
    }
    if (keys[SDL_SCANCODE_D])
    {
        (*dx)++;
    }
}
