#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <nlohmann/json.hpp>

#include <cassert>
#include <exception>
#include <fstream>
#include <print>
#include <string>

#include "gpu_loaders.hpp"

static SDL_GPUShaderFormat get_shader_format(SDL_GPUDevice* device)
{
    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        return SDL_GPU_SHADERFORMAT_SPIRV;
    }
    else if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        return SDL_GPU_SHADERFORMAT_DXIL;
    }
    else if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_MSL)
    {
        return SDL_GPU_SHADERFORMAT_MSL;
    }
    else
    {
        assert(false);
    }
    return 0;
}

static const char* get_shader_entrypoint(SDL_GPUDevice* device)
{
    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        return "main";
    }
    else if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        return "main";
    }
    else if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_MSL)
    {
        return "main0";
    }
    else
    {
        assert(false);
    }
    return "";
}

static const char* get_shader_string(SDL_GPUDevice* device)
{
    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        return ".spv";
    }
    else if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        return ".dxil";
    }
    else if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_MSL)
    {
        return ".msl";
    }
    else
    {
        assert(false);
    }
    return "";
}

SDL_GPUShader* load_gpu_shader(SDL_GPUDevice* device, const char* name)
{
    std::string path = name;
    std::string shader_path = path + get_shader_string(device);
    std::string json_path = path + ".json";

    SDL_GPUShaderCreateInfo info{};

    void* code = SDL_LoadFile(shader_path.data(), &info.code_size);
    info.code = static_cast<uint8_t*>(code);
    if (!code)
    {
        std::println("Failed to load shader: {}, {}", shader_path, SDL_GetError());
        return nullptr;
    }

    std::fstream file(json_path);
    if (!file.good())
    {
        std::println("Failed to open json: {}", json_path);
        SDL_free(code);
        return nullptr;
    }

    nlohmann::json json;
    try
    {
        json << file;
    }
    catch (const std::exception& e)
    {
        std::println("Failed to load json: {}, {}", json_path, e.what());
        SDL_free(code);
        return nullptr;
    }

    info.num_uniform_buffers = json["uniform_buffers"];
    info.num_samplers = json["samplers"];
    info.num_storage_buffers = json["storage_buffers"];
    info.num_storage_textures = json["storage_textures"];

    if (path.contains(".vert"))
    {
        info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (path.contains(".frag"))
    {
        info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        assert(false);
    }

    info.format = get_shader_format(device);
    info.entrypoint = get_shader_entrypoint(device);

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    SDL_free(code);
    if (!shader)
    {
        std::println("Failed to create shader: {}, {}", shader_path, SDL_GetError());
        return nullptr;
    }

    return shader;
}

SDL_GPUComputePipeline* load_gpu_compute_pipeline(SDL_GPUDevice* device, const char* name)
{
    std::string path = name;
    std::string shader_path = path + get_shader_string(device);
    std::string json_path = path + ".json";

    SDL_GPUComputePipelineCreateInfo info{};

    void* code = SDL_LoadFile(shader_path.data(), &info.code_size);
    info.code = static_cast<uint8_t*>(code);
    if (!code)
    {
        std::println("Failed to load shader: {}, {}", shader_path, SDL_GetError());
        return nullptr;
    }

    std::fstream file(json_path);
    if (!file.good())
    {
        std::println("Failed to open json: {}", json_path);
        SDL_free(code);
        return nullptr;
    }

    nlohmann::json json;
    try
    {
        json << file;
    }
    catch (const std::exception& e)
    {
        std::println("Failed to load json: {}, {}", json_path, e.what());
        SDL_free(code);
        return nullptr;
    }

    info.num_uniform_buffers = json["uniform_buffers"];
    info.num_samplers = json["samplers"];
    info.num_readwrite_storage_buffers = json["readwrite_storage_buffers"];
    info.num_readwrite_storage_textures = json["readwrite_storage_textures"];
    info.num_readonly_storage_buffers = json["readonly_storage_buffers"];
    info.num_readonly_storage_textures = json["readonly_storage_textures"];

    info.threadcount_x = json["threadcount_x"];
    info.threadcount_y = json["threadcount_y"];
    info.threadcount_z = json["threadcount_z"];

    info.format = get_shader_format(device);
    info.entrypoint = get_shader_entrypoint(device);

    SDL_GPUComputePipeline* shader = SDL_CreateGPUComputePipeline(device, &info);
    SDL_free(code);
    if (!shader)
    {
        std::println("Failed to create compute pipeline: {}, {}", shader_path, SDL_GetError());
        return nullptr;
    }

    return shader;
}

SDL_GPUTexture* load_gpu_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path)
{
    SDL_Surface* surface = IMG_Load(path);
    if (!surface)
    {
        std::println("Failed to load surface: {}, {}", path, SDL_GetError());
        return nullptr;
    }

    SDL_GPUTextureCreateInfo info{};

    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    info.width = surface->w;
    info.height = surface->h;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &info);
    if (!texture)
    {
        std::println("Failed to create texture: {}, {}", path, SDL_GetError());
        SDL_DestroySurface(surface);
        return nullptr;
    }

    SDL_GPUTransferBufferCreateInfo tbci{};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size = surface->w * surface->h * 4;
    SDL_GPUTransferBuffer* buffer = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!buffer)
    {
        std::println("Failed to create transfer buffer: {}, {}", path, SDL_GetError());
        SDL_DestroySurface(surface);
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }

    void* data = SDL_MapGPUTransferBuffer(device, buffer, false);
    if (!data)
    {
        std::println("Failed to map transfer buffer: {}, {}", path, SDL_GetError());
        SDL_DestroySurface(surface);
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }

    memcpy(data, surface->pixels, surface->w * surface->h * 4);
    SDL_UnmapGPUTransferBuffer(device, buffer);

    SDL_GPUTextureTransferInfo transfer_info = {0};
    SDL_GPUTextureRegion region = {0};
    transfer_info.transfer_buffer = buffer;
    region.texture = texture;
    region.w = surface->w;
    region.h = surface->h;
    region.d = 1;

    SDL_DestroySurface(surface);

    SDL_UploadToGPUTexture(copy_pass, &transfer_info, &region, true);
    SDL_ReleaseGPUTransferBuffer(device, buffer);

    return texture;
}