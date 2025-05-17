#include <SDL3/SDL.h>

#include <tiny_obj_loader.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

#include "loaders.hpp"
#include "model.hpp"
#include "voxel.hpp"

bool Model::load(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const std::string& name)
{
    /* TODO: refactor (too many copies) */
    /* TODO: fix leaks on failure */

    std::string obj_path = name + ".obj";
    std::string png_path = name + ".png";

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(obj_path))
    {
        std::println("Failed to load model: {}, {}", obj_path, reader.Error());
        return false;
    }

    tinyobj::attrib_t attrib = reader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
    std::vector<uint16_t> indices;
    std::vector<Voxel> vertices;
    std::unordered_map<Voxel, uint32_t> unique_vertices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            static constexpr int scale = 10;

            int vx = attrib.vertices[3 * index.vertex_index + 0] * scale;
            int vy = attrib.vertices[3 * index.vertex_index + 1] * scale;
            int vz = attrib.vertices[3 * index.vertex_index + 2] * scale;
            int tx = attrib.texcoords[2 * index.texcoord_index + 0];
            int nx = attrib.normals[3 * index.normal_index + 0];
            int ny = attrib.normals[3 * index.normal_index + 1];
            int nz = attrib.normals[3 * index.normal_index + 2];

            Voxel voxel{nx, vy, vz, tx, nx, ny, nz};

            auto it = unique_vertices.find(voxel);
            if (it == unique_vertices.end())
            {
                uint32_t size = static_cast<uint32_t>(vertices.size());
                it = unique_vertices.emplace(voxel, size).first;
                vertices.emplace_back(voxel);
            }
            indices.push_back(it->second);
        }
    }

    palette = load_texture(device, copy_pass, png_path);
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
        info.size = vertices.size() * sizeof(Voxel);
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
        info.size = vertices.size() * sizeof(Voxel);
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

    std::memcpy(vertex_data, vertices.data(), vertices.size() * sizeof(Voxel));
    std::memcpy(index_data, indices.data(), indices.size() * sizeof(uint16_t));

    SDL_UnmapGPUTransferBuffer(device, vertex_transfer_buffer);
    SDL_UnmapGPUTransferBuffer(device, index_transfer_buffer);

    SDL_GPUBufferRegion region{};
    SDL_GPUTransferBufferLocation location{};

    region.buffer = vertex_buffer;
    region.size = vertices.size() * sizeof(Voxel);
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

void Model::free(SDL_GPUDevice* device)
{
    if (palette)
    {
        SDL_ReleaseGPUTexture(device, palette);
        palette = nullptr;
    }

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
}

SDL_GPUTexture* Model::get_palette()
{
    return palette;
}

SDL_GPUBuffer* Model::get_vertex_buffer()
{
    return vertex_buffer;
}

SDL_GPUBuffer* Model::get_index_buffer()
{
    return index_buffer;
}

int Model::get_num_indices() const
{
    return num_indices;
}