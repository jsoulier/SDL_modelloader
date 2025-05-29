#pragma once

#include <SDL3/SDL.h>

#define trace(...) SDL_Log("[TRACE] " __VA_ARGS__)
#define warning(...) SDL_Log("[WARNING] " __VA_ARGS__)
#define error(...) SDL_Log("[ERROR] " __VA_ARGS__)

SDL_GPUTexture* load_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path);