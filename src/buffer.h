#pragma once

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct buffer
{
    const char* name;

    SDL_GPUTransferBuffer* transfer_buffer;
    SDL_GPUBuffer* buffer;
    SDL_GPUBufferUsageFlags buffer_usage;

    uint8_t* data;

    uint32_t stride;
    uint32_t size;
    uint32_t capacity;

    bool resize;
}
buffer_t;

void buffer_init(buffer_t* buffer, SDL_GPUBufferUsageFlags buffer_usage, uint32_t stride, const char* name);
void buffer_free(buffer_t* buffer, SDL_GPUDevice* device);
void buffer_append(buffer_t* buffer, SDL_GPUDevice* device, void* data);
void buffer_upload(buffer_t* buffer, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);