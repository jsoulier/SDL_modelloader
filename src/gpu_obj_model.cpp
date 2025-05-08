#include <SDL3/SDL.h>

#include <tiny_obj_loader.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

#include "gpu_loaders.hpp"
#include "gpu_obj_model.hpp"
#include "packed_vertex.hpp"

bool gpu_obj_model::load(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* name)
{
    /* TODO: refactor */
    /* TODO: fix leaks on failure */

    std::string path = name;
    std::string obj_path = path + ".obj";
    std::string png_path = path + ".png";

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(obj_path))
    {
        std::println("Failed to load model: {}, {}", obj_path, reader.Error());
        return false;
    }

    tinyobj::attrib_t attrib = reader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
    std::vector<uint16_t> indices;
    std::vector<packed_vertex> vertices;
    std::unordered_map<packed_vertex, uint32_t> unique_vertices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            static constexpr int int_scale = 10;

            int vx = attrib.vertices[3 * index.vertex_index + 0] * int_scale;
            int vy = attrib.vertices[3 * index.vertex_index + 1] * int_scale;
            int vz = attrib.vertices[3 * index.vertex_index + 2] * int_scale;
            int tx = attrib.texcoords[2 * index.texcoord_index + 0];
            int nx = attrib.normals[3 * index.normal_index + 0];
            int ny = attrib.normals[3 * index.normal_index + 1];
            int nz = attrib.normals[3 * index.normal_index + 2];

            packed_vertex vertex{nx, vy, vz, tx, nx, ny, nz};

            auto it = unique_vertices.find(vertex);
            if (it == unique_vertices.end())
            {
                uint32_t size = static_cast<uint32_t>(vertices.size());
                it = unique_vertices.emplace(vertex, size).first;
                vertices.emplace_back(vertex);
            }
            indices.push_back(it->second);
        }
    }

    palette = load_gpu_texture(device, copy_pass, png_path.data());
    if (!palette)
    {
        std::println("Failed to load palette: {}", png_path);
        return false;
    }

    SDL_GPUTransferBuffer* vertex_transfer_buffer;
    SDL_GPUTransferBuffer* index_transfer_buffer;

    {
        SDL_GPUTransferBufferCreateInfo info{};

        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = vertices.size() * sizeof(packed_vertex);
        vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);

        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = indices.size() * sizeof(uint16_t);
        index_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);

        if (!vertex_transfer_buffer || !index_transfer_buffer)
        {
            std::println("Failed to create transfer buffer(s): {}", SDL_GetError());
            return false;
        }
    }

    {
        SDL_GPUBufferCreateInfo info{};

        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = vertices.size() * sizeof(packed_vertex);
        vertex_buffer = SDL_CreateGPUBuffer(device, &info);

        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = indices.size() * sizeof(uint16_t);
        index_buffer = SDL_CreateGPUBuffer(device, &info);

        if (!vertex_buffer || !index_buffer)
        {
            std::println("Failed to create buffer(s): {}", SDL_GetError());
            return false;
        }
    }

    void* vertex_data = SDL_MapGPUTransferBuffer(device, vertex_transfer_buffer, false);
    void* index_data = SDL_MapGPUTransferBuffer(device, index_transfer_buffer, false);
    if (!vertex_data || !index_data)
    {
        std::println("Failed to map buffer(s): {}", SDL_GetError());
        return false;
    }

    std::memcpy(vertex_data, vertices.data(), vertices.size() * sizeof(packed_vertex));
    std::memcpy(index_data, indices.data(), indices.size() * sizeof(uint16_t));

    SDL_UnmapGPUTransferBuffer(device, vertex_transfer_buffer);
    SDL_UnmapGPUTransferBuffer(device, index_transfer_buffer);

    SDL_GPUBufferRegion region{};
    SDL_GPUTransferBufferLocation location{};

    region.buffer = vertex_buffer;
    region.size = vertices.size() * sizeof(packed_vertex);
    location.transfer_buffer = vertex_transfer_buffer;
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

    region.buffer = index_buffer;
    region.size = indices.size() * sizeof(uint16_t);
    location.transfer_buffer = index_transfer_buffer;
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

    SDL_ReleaseGPUTransferBuffer(device, vertex_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(device, index_transfer_buffer);

    assert(indices.size() < std::numeric_limits<uint16_t>::max());
    num_indices = indices.size();

    return true;
}

void gpu_obj_model::free(SDL_GPUDevice* device)
{
    if (vertex_buffer)
    {
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        vertex_buffer = nullptr;
    }

    if (index_buffer)
    {
        SDL_ReleaseGPUBuffer(device, index_buffer);
        index_buffer = nullptr;
    }

    if (palette)
    {
        SDL_ReleaseGPUTexture(device, palette);
        palette = nullptr;
    }
}

SDL_GPUBuffer* gpu_obj_model::get_vertex_buffer()
{
    return vertex_buffer;
}

SDL_GPUBuffer* gpu_obj_model::get_index_buffer()
{
    return index_buffer;
}

SDL_GPUTexture* gpu_obj_model::get_palette()
{
    return palette;
}

int gpu_obj_model::get_num_indices() const
{
    return num_indices;
}