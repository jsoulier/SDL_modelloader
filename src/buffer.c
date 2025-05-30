#include <SDL3/SDL.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "util.h"

void buffer_init(buffer_t* buffer, SDL_GPUBufferUsageFlags buffer_usage, uint32_t stride, const char* name)
{
    assert(buffer);
    assert(stride);

    buffer->name = name;

    buffer->transfer_buffer = NULL;
    buffer->buffer = NULL;
    buffer->buffer_usage = buffer_usage;

    buffer->data = NULL;

    buffer->stride = stride;
    buffer->size = 0;
    buffer->capacity = 0;

    buffer->resize = false;
}

void buffer_free(buffer_t* buffer, SDL_GPUDevice* device)
{
    assert(buffer);
    assert(device);

    assert(!buffer->data);

    if (buffer->transfer_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(device, buffer->transfer_buffer);
        buffer->transfer_buffer = NULL;
    }

    if (buffer->buffer)
    {
        SDL_ReleaseGPUBuffer(device, buffer->buffer);
        buffer->buffer = NULL;
    }
}

void buffer_append(buffer_t* buffer, SDL_GPUDevice* device, void* data)
{
    assert(buffer);
    assert(device);
    assert(data);

    if (!buffer->data && buffer->transfer_buffer)
    {
        buffer->size = 0;
        buffer->resize = false;

        buffer->data = SDL_MapGPUTransferBuffer(device, buffer->transfer_buffer, true);
        if (!buffer->data)
        {
            error("Failed to map transfer buffer: %s, %s", buffer->name, SDL_GetError());
            return;
        }
    }

    if (buffer->size == buffer->capacity)
    {
        const uint32_t starting_capacity = 20;
        const float growth_rate = 2.0f;

        uint32_t capacity;
        if (buffer->size)
        {
            capacity = buffer->size * growth_rate;
        }
        else
        {
            capacity = starting_capacity;
        }

        SDL_GPUTransferBufferCreateInfo info = {0};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = capacity * buffer->stride;

        if (is_debugging)
        {
            info.props = SDL_CreateProperties();
            if (!info.props)
            {
                error("Failed to create properties: %s, %s", buffer->name, SDL_GetError());
                return;
            }

            SDL_SetStringProperty(info.props, "SDL.gpu.transferbuffer.create.name", buffer->name);
        }

        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        SDL_DestroyProperties(info.props);
        if (!transfer_buffer)
        {
            error("Failed to create transfer buffer: %s, %s", buffer->name, SDL_GetError());
            return;
        }

        uint8_t* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        if (!data)
        {
            error("Failed to map transfer buffer: %s, %s", buffer->name, SDL_GetError());
            SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
            return;
        }

        if (buffer->transfer_buffer)
        {
            if (buffer->data)
            {
                memcpy(data, buffer->data, buffer->size * buffer->stride);
                SDL_UnmapGPUTransferBuffer(device, buffer->transfer_buffer);
            }

            SDL_ReleaseGPUTransferBuffer(device, buffer->transfer_buffer);
        }

        buffer->capacity = capacity;
        buffer->transfer_buffer = transfer_buffer;
        buffer->data = data;
        buffer->resize = true;
    }

    memcpy(buffer->data + buffer->size * buffer->stride, data, buffer->stride);
    buffer->size++;
}

void buffer_upload(buffer_t* buffer, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    assert(buffer);
    assert(device);
    assert(copy_pass);

    if (buffer->data)
    {
        SDL_UnmapGPUTransferBuffer(device, buffer->transfer_buffer);
        buffer->data = NULL;
    }

    if (buffer->size == 0)
    {
        return;
    }

    if (buffer->resize)
    {
        if (buffer->buffer)
        {
            SDL_ReleaseGPUBuffer(device, buffer->buffer);
            buffer->buffer = NULL;
        }

        SDL_GPUBufferCreateInfo info = {0};
        info.usage = buffer->buffer_usage;
        info.size = buffer->capacity * buffer->stride;

        if (is_debugging)
        {
            info.props = SDL_CreateProperties();
            if (!info.props)
            {
                error("Failed to create properties: %s, %s", buffer->name, SDL_GetError());
                return;
            }

            SDL_SetStringProperty(info.props, "SDL.gpu.buffer.create.name", buffer->name);
        }

        buffer->buffer = SDL_CreateGPUBuffer(device, &info);
        SDL_DestroyProperties(info.props);
        if (!buffer->buffer)
        {
            error("Failed to create buffer: %s, %s", buffer->name, SDL_GetError());
            return;
        }

        buffer->resize = false;
    }

    SDL_GPUTransferBufferLocation location = {0};
    location.transfer_buffer = buffer->transfer_buffer;

    SDL_GPUBufferRegion region = {0};
    region.buffer = buffer->buffer;
    region.size = buffer->size * buffer->stride;

    SDL_UploadToGPUBuffer(copy_pass, &location, &region, true);
}