#pragma once

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdint.h>

typedef enum mesh_type
{
    mesh_count,
}
mesh_type_t;

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
    mesh_index_t num_indices;
    uint32_t num_vertices;
}
mesh_t;

bool mesh_load(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, mesh_t* mesh, const char* name);
void mesh_free(SDL_GPUDevice* device, mesh_t* mesh);