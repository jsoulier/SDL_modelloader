#pragma once

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdint.h>

#ifndef NDEBUG
static const bool is_debugging = true;
#else
static const bool is_debugging = false;
#endif

#define trace(...) SDL_Log("[TRACE] " __VA_ARGS__)
#define warning(...) SDL_Log("[WARNING] " __VA_ARGS__)
#define error(...) SDL_Log("[ERROR] " __VA_ARGS__)

SDL_GPUShader* load_shader(SDL_GPUDevice* device, const char* name);
SDL_GPUTexture* load_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path);

void push_debug_group(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, const char* name);
void pop_debug_group(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer);