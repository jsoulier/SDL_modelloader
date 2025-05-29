#pragma once

#include <SDL3/SDL.h>

#include <stdbool.h>

#ifndef NDEBUG
static const bool is_debugging = true;
#else
static const bool is_debugging = false;
#endif

#define trace(...) SDL_Log("[TRACE] " __VA_ARGS__)
#define warning(...) SDL_Log("[WARNING] " __VA_ARGS__)
#define error(...) SDL_Log("[ERROR] " __VA_ARGS__)

SDL_GPUTexture* load_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path);