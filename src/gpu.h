#pragma once

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdint.h>

SDL_GPUShader* load_shader(SDL_GPUDevice* device, const char* name);
SDL_GPUComputePipeline* load_compute_pipeline(SDL_GPUDevice* device, const char* name);
SDL_GPUTexture* load_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path);

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

typedef uint16_t mesh_index_t;
typedef struct mesh_vertex
{
    uint32_t packed;
    float texcoord;
}
mesh_vertex_t;

typedef struct mesh
{
    SDL_GPUTexture* palette;
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;

    uint32_t num_vertices;
    mesh_index_t num_indices;
}
mesh_t;

void buffer_init(buffer_t* buffer, SDL_GPUBufferUsageFlags buffer_usage, uint32_t stride, const char* name);
void buffer_free(buffer_t* buffer, SDL_GPUDevice* device);
void buffer_append(buffer_t* buffer, SDL_GPUDevice* device, const void* data);
void buffer_upload(buffer_t* buffer, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
bool mesh_load(mesh_t* mesh, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* name);
void mesh_free(mesh_t* mesh, SDL_GPUDevice* device);