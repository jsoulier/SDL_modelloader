#pragma once

#include <SDL3/SDL.h>

#include <assert.h>
#undef assert

#ifndef NDEBUG
#define is_debugging 1
#define assert SDL_assert
#define log_release SDL_Log
#define log_debug SDL_Log
#else
#define is_debugging 0
#define assert
#define log_release SDL_Log
#define log_debug
#endif

void log_init();
void log_quit();
void push_debug_group(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, const char* name);
void insert_debug_label(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, const char* name);
void pop_debug_group(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer);