#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <cstring>
#include <print>

template<typename T>
class Buffer
{
public:
    Buffer()
        : transfer_buffer{nullptr}
        , buffer_usage{SDL_GPU_BUFFERUSAGE_VERTEX}
        , buffer{nullptr}
        , data{nullptr}
        , size{0}
        , capacity{0}
        , resize{false} {}

    void free(SDL_GPUDevice* device)
    {
        if (transfer_buffer)
        {
            SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
            transfer_buffer = nullptr;
        }

        if (buffer)
        {
            SDL_ReleaseGPUBuffer(device, buffer);
            buffer = nullptr;
        }
    }

    void push_back(SDL_GPUDevice* device, const T& instance)
    {
        if (!data && transfer_buffer)
        {
            size = 0;
            resize = false;

            data = static_cast<T*>(SDL_MapGPUTransferBuffer(device, transfer_buffer, true));
            if (!data)
            {
                std::println("Failed to map transfer buffer: {}", SDL_GetError());
                return;
            }
        }

        if (size == capacity)
        {
            static constexpr uint32_t starting_capacity = 20;
            static constexpr float capacity_growth_rate = 2.0f;

            uint32_t new_capacity;
            if (size)
            {
                new_capacity = size * capacity_growth_rate;
            }
            else
            {
                new_capacity = starting_capacity;
            }

            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = new_capacity * sizeof(T);
            SDL_GPUTransferBuffer* new_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!new_transfer_buffer)
            {
                std::println("Failed to create transfer buffer: {}", SDL_GetError());
                return;
            }

            T* new_data = static_cast<T*>(SDL_MapGPUTransferBuffer(device, new_transfer_buffer, false));
            if (!new_data)
            {
                std::println("Failed to map transfer buffer: {}", SDL_GetError());
                SDL_ReleaseGPUTransferBuffer(device, new_transfer_buffer);
                return;
            }

            if (data)
            {
                std::memcpy(new_data, data, size * sizeof(T));
                SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
            }

            if (transfer_buffer)
            {
                SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
            }

            transfer_buffer = new_transfer_buffer;
            data = new_data;
            capacity = new_capacity;
            resize = true;
        }

        data[size++] = instance;
    }

    void upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
    {
        if (data)
        {
            SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
            data = nullptr;
        }

        if (size == 0)
        {
            return;
        }

        if (resize)
        {
            if (buffer)
            {
                SDL_ReleaseGPUBuffer(device, buffer);
            }

            SDL_GPUBufferCreateInfo info{};
            info.usage = buffer_usage;
            info.size = capacity * sizeof(T);
            buffer = SDL_CreateGPUBuffer(device, &info);
            if (!buffer)
            {
                std::println("Failed to create buffer: {}", SDL_GetError());
                return;
            }

            resize = false;
        }

        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};
        location.transfer_buffer = transfer_buffer;
        region.buffer = buffer;
        region.size = size * sizeof(T);

        SDL_UploadToGPUBuffer(copy_pass, &location, &region, true);
    }

    SDL_GPUBuffer* get_buffer()
    {
        return buffer;
    }

    uint32_t get_size() const
    {
        return size;
    }

private:
    SDL_GPUTransferBuffer* transfer_buffer;
    SDL_GPUBufferUsageFlags buffer_usage;
    SDL_GPUBuffer* buffer;

    T* data;

    uint32_t size;
    uint32_t capacity;

    bool resize;
};