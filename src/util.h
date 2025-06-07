#pragma once

#include <SDL3/SDL.h>

#ifndef NDEBUG
#define is_debugging 1
#define log_release SDL_Log
#define log_debug SDL_Log
#define assert_release SDL_assert_always
#define assert_debug SDL_assert_always
#else
#define is_debugging 0
#define log_release SDL_Log
#define log_debug
#define assert_release SDL_assert_always
#define assert_debug
#endif

void log_init();
void log_quit();
void push_debug_group(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, const char* name);
void insert_debug_label(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, const char* name);
void pop_debug_group(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer);