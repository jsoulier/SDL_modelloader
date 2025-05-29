#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <fast_obj.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mesh.h"
#include "util.h"

bool mesh_load(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, mesh_t* mesh, const char* name)
{
    /* TODO: fix leaks on error */

    assert(device);
    assert(copy_pass);
    assert(mesh);
    assert(name);

    char obj_path[256] = {0};
    char png_path[256] = {0};

    snprintf(obj_path, sizeof(obj_path), "%s.obj", name);
    snprintf(png_path, sizeof(png_path), "%s.png", name);

    fastObjMesh* obj_mesh = fast_obj_read(obj_path);
    if (!obj_mesh)
    {
        error("Failed to load mesh: %s", obj_path);
        return false;
    }

    SDL_GPUTransferBuffer* vertex_buffer;
    SDL_GPUTransferBuffer* index_buffer;

    {
        SDL_GPUTransferBufferCreateInfo info = {0};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;

        info.size = obj_mesh->index_count * sizeof(mesh_vertex_t);
        vertex_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!vertex_buffer)
        {
            error("Failed to create vertex transfer buffer: %s", SDL_GetError());
            return false;
        }

        info.size = obj_mesh->index_count * sizeof(mesh_index_t);
        index_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!index_buffer)
        {
            error("Failed to create index transfer buffer: %s", SDL_GetError());
            return false;
        }
    }

    mesh_vertex_t* vertex_data = SDL_MapGPUTransferBuffer(device, vertex_buffer, false);
    if (!vertex_data)
    {
        error("Failed to map vertex transfer buffer: %s", SDL_GetError());
        return false;
    }

    mesh_index_t* index_data = SDL_MapGPUTransferBuffer(device, index_buffer, false);
    if (!index_data)
    {
        error("Failed to map index transfer buffer: %s", SDL_GetError());
        return false;
    }

    struct
    {
        mesh_vertex_t key;
        int value;
    }
    *vertex_to_index = NULL;
    stbds_hmdefault(vertex_to_index, -1);
    if (!vertex_to_index)
    {
        error("Failed to create map");
        return false;
    }

    mesh->num_indices = obj_mesh->index_count;
    mesh->num_vertices = 0;

    for (uint32_t i = 0; i < obj_mesh->index_count; i++)
    {
        uint32_t position_index = obj_mesh->indices[i].p;
        uint32_t texcoord_index = obj_mesh->indices[i].t;
        uint32_t normal_index = obj_mesh->indices[i].n;

        if (position_index <= 0)
        {
            error("Missing position data: %s", name);
            return false;
        }

        if (texcoord_index <= 0)
        {
            error("Missing texcoord data: %s", name);
            return false;
        }

        if (normal_index <= 0)
        {
            error("Missing normal data: %s", name);
            return false;
        }

        const float scale = 10.0f;

        mesh_vertex_t vertex = {0};

        int position_x = obj_mesh->positions[position_index * 3 + 0] * scale;
        int position_y = obj_mesh->positions[position_index * 3 + 1] * scale;
        int position_z = obj_mesh->positions[position_index * 3 + 2] * scale;

        vertex.texcoord = obj_mesh->texcoords[texcoord_index * 2 + 0];

        int normal_x = obj_mesh->normals[normal_index * 3 + 0];
        int normal_y = obj_mesh->normals[normal_index * 3 + 1];
        int normal_z = obj_mesh->normals[normal_index * 3 + 2];

        assert(position_x >= -16 && position_x <= 16);
        assert(position_y >= -16 && position_y <= 16);
        assert(position_z >= -16 && position_z <= 16);

        vertex.packed |= (abs(position_x) & 0x7F) << 0;
        vertex.packed |= (position_x < 0) << 7;
        vertex.packed |= (abs(position_y) & 0x7F) << 8;
        vertex.packed |= (position_y < 0) << 15;
        vertex.packed |= (abs(position_z) & 0x7F) << 16;
        vertex.packed |= (position_z < 0) << 23;

        uint32_t normal = 0;
        if (normal_x > 0)
        {
            normal = 0;
        }
        else if (normal_x < 0)
        {
            normal = 1;
        }
        else if (normal_y > 0)
        {
            normal = 2;
        }
        else if (normal_y < 0)
        {
            normal = 3;
        }
        else if (normal_z > 0)
        {
            normal = 4;
        }
        else if (normal_z < 0)
        {
            normal = 5;
        }
        else
        {
            assert(false);
        }
        vertex.packed |= (normal & 0x7) << 24;

        const int index = stbds_hmget(vertex_to_index, vertex);
        if (index == -1)
        {
            stbds_hmput(vertex_to_index, vertex, mesh->num_vertices);
            vertex_data[mesh->num_vertices] = vertex;
            index_data[i] = mesh->num_vertices++;
        }
        else
        {
            index_data[i] = index;
        }
    }

    SDL_UnmapGPUTransferBuffer(device, vertex_buffer);
    SDL_UnmapGPUTransferBuffer(device, index_buffer);

    {
        SDL_GPUBufferCreateInfo info = {0};

        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = mesh->num_vertices * sizeof(mesh_vertex_t);
        mesh->vertex_buffer = SDL_CreateGPUBuffer(device, &info);
        if (!mesh->vertex_buffer)
        {
            error("Failed to create vertex buffer: %s", SDL_GetError());
            return false;
        }

        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = mesh->num_indices * sizeof(mesh_index_t);
        mesh->index_buffer = SDL_CreateGPUBuffer(device, &info);
        if (!mesh->index_buffer)
        {
            error("Failed to create index buffer: %s", SDL_GetError());
            return false;
        }    
    }

    SDL_GPUTransferBufferLocation location = {0};
    SDL_GPUBufferRegion region = {0};

    location.transfer_buffer = vertex_buffer;
    region.buffer = mesh->vertex_buffer;
    region.size = mesh->num_vertices * sizeof(mesh_vertex_t);
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

    location.transfer_buffer = index_buffer;
    region.buffer = mesh->index_buffer;
    region.size = mesh->num_vertices * sizeof(mesh_index_t);
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

    SDL_ReleaseGPUTransferBuffer(device, vertex_buffer);
    SDL_ReleaseGPUTransferBuffer(device, index_buffer);

    stbds_hmfree(vertex_to_index);
    fast_obj_destroy(obj_mesh);

    mesh->palette = load_texture(device, copy_pass, png_path);
    if (!mesh->palette)
    {
        error("Failed to load palette: %s", png_path);
        return false;
    }

    return true;
}

void mesh_free(SDL_GPUDevice* device, mesh_t* mesh)
{
    assert(device);
    assert(mesh);

    if (mesh->palette)
    {
        SDL_ReleaseGPUTexture(device, mesh->palette);
        mesh->palette = NULL;
    }

    if (mesh->vertex_buffer)
    {
        SDL_ReleaseGPUBuffer(device, mesh->vertex_buffer);
        mesh->vertex_buffer = NULL;
    }

    if (mesh->index_buffer)
    {
        SDL_ReleaseGPUBuffer(device, mesh->index_buffer);
        mesh->index_buffer = NULL;
    }
}