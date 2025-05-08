#include <SDL3/SDL.h>

#include <stddef.h>

#include "dbg.h"

static void log_callback(void* data, int category, SDL_LogPriority priority, const char* string)
{
    if (is_debugging)
    {
        SDL_GetDefaultLogOutputFunction()(data, category, priority, string);
    }
    if (data)
    {
        SDL_IOprintf(data, "%s\n", string);
    }
}

void log_init()
{
    SDL_IOStream* iostream = SDL_IOFromFile("lilcraft.log", "w");
    if (!iostream)
    {
        log_release("Failed to open iostream: %s", SDL_GetError());
    }

    SDL_SetLogOutputFunction(log_callback, iostream);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
}

void log_quit()
{
    SDL_IOStream* iostream;
    SDL_GetLogOutputFunction(NULL, &iostream);
    SDL_SetLogOutputFunction(SDL_GetDefaultLogOutputFunction(), NULL);

    if (iostream)
    {
        SDL_CloseIO(iostream);
    }
}

void push_debug_group(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, const char* name)
{
    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        return;
    }
    if (is_debugging)
    {
        SDL_PushGPUDebugGroup(command_buffer, name);
    }
}

void insert_debug_label(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, const char* name)
{
    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        return;
    }
    if (is_debugging)
    {
        SDL_InsertGPUDebugLabel(command_buffer, name);
    }
}

void pop_debug_group(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer)
{
    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        return;
    }
    if (is_debugging)
    {
        SDL_PopGPUDebugGroup(command_buffer);
    }
}