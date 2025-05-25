#include <SDL3/SDL.h>

#include "entity/player.hpp"
#include "math.hpp"
#include "renderer.hpp"

glm::vec3 PlayerEntity::get_motion() const
{
    glm::vec3 motion{};

    const bool* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_W])
    {
        motion.z--;
    }
    if (keys[SDL_SCANCODE_S])
    {
        motion.z++;
    }
    if (keys[SDL_SCANCODE_A])
    {
        motion.x--;
    }
    if (keys[SDL_SCANCODE_D])
    {
        motion.x++;
    }

    return motion;
}

ModelId PlayerEntity::get_model() const
{
    return MODEL_PLAYER;
}

EntityType PlayerEntity::get_type() const
{
    return ENTITY_PLAYER;
}