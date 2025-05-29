#include <SDL3/SDL.h>
#include <jsmn.h>
#include <stb_image.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

SDL_GPUShader* load_shader(SDL_GPUDevice* device, const char* name)
{
    /* TODO: fix leaks on error */

    assert(device);
    assert(name);

    SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(device);
    const char* entrypoint;
    const char* file_extension;

    if (format & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
        file_extension = "spv";
    }
    else if (format & SDL_GPU_SHADERFORMAT_DXIL)
    {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
        file_extension = "dxil";
    }
    else if (format & SDL_GPU_SHADERFORMAT_MSL)
    {
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
        file_extension = "msl";
    }
    else
    {
        assert(false);
    }

    char shader_path[256] = {0};
    char shader_json_path[256] = {0};

    snprintf(shader_path, sizeof(shader_path), "%s.%s", name, file_extension);
    snprintf(shader_json_path, sizeof(shader_json_path), "%s.json", name);

    size_t shader_size;
    size_t shader_json_size;

    char* shader_data = SDL_LoadFile(shader_path, &shader_size);
    if (!shader_data)
    {
        error("Failed to load shader: %s", shader_path);
        return NULL;
    }

    char* shader_json_data = SDL_LoadFile(shader_json_path, &shader_json_size);
    if (!shader_json_data)
    {
        error("Failed to load shader json: %s", shader_json_path);
        return NULL;
    }

    jsmn_parser json_parser;
    jsmntok_t json_tokens[8];

    jsmn_init(&json_parser);
    if (jsmn_parse(&json_parser, shader_json_data, shader_json_size, json_tokens, 8) <= 0)
    {
        error("Failed to parse json: %s", shader_json_path);
        return NULL;
    }

    SDL_GPUShaderCreateInfo info = {0};

    for (int i = 0; i < 8; i += 2)
    {
        if (json_tokens[i].type != JSMN_STRING)
        {
            error("Bad json type: %s", shader_json_path);
            return NULL;
        }

        char* key_string = shader_json_data + json_tokens[i + 0].start;
        char* value_string = shader_json_data + json_tokens[i + 1].start;
        int key_size = json_tokens[i].size;

        uint32_t* value;

        if (!memcmp("num_samplers", key_string, key_size))
        {
            value = &info.num_samplers;
        }
        else if (!memcmp("num_storage_textures", key_string, key_size))
        {
            value = &info.num_storage_textures;
        }
        else if (!memcmp("num_storage_buffers", key_string, key_size))
        {
            value = &info.num_storage_buffers;
        }
        else if (!memcmp("num_uniform_buffers", key_string, key_size))
        {
            value = &info.num_uniform_buffers;
        }

        *value = *value_string - '0';
    }

    info.code = shader_data;
    info.code_size = shader_size;

    info.entrypoint = entrypoint;
    info.format = format;

    if (strstr(name, ".frag"))
    {
        info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }

    if (is_debugging)
    {
        info.props = SDL_CreateProperties();
    }

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    if (!shader)
    {
        error("Failed to create shader: %s", SDL_GetError());
        return NULL;
    }
        
    if (is_debugging)
    {
        SDL_DestroyProperties(info.props);
    }

    SDL_free(shader_data);
    SDL_free(shader_json_data);

    return shader;
}

SDL_GPUTexture* load_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path)
{
    /* TODO: fix leaks on error */

    assert(device);
    assert(copy_pass);
    assert(path);

    int channels;
    int width;
    int height;

    void* src_data = stbi_load(path, &width, &height, &channels, 4);
    if (!src_data)
    {
        error("Failed to load image: %s, %s", path, stbi_failure_reason());
        return NULL;
    }

    SDL_GPUTransferBuffer* transfer_buffer;
    SDL_GPUTexture* texture;

    {
        SDL_GPUTransferBufferCreateInfo info = {0};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = width * height * 4;
        transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transfer_buffer)
        {
            error("Failed to create transfer buffer: %s", SDL_GetError());
            return NULL;
        }
    }

    void* dst_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!dst_data)
    {
        error("Failed to map transfer buffer: %s", SDL_GetError());
        return NULL;
    }

    memcpy(dst_data, src_data, width * height * 4);
    stbi_image_free(src_data);

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    {
        SDL_GPUTextureCreateInfo info = {0};

        info.props = SDL_CreateProperties();

        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;

        info.num_levels = 1;

        if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
        {
            SDL_SetFloatProperty(info.props, "SDL.gpu.texture.create.d3d12.clear.depth", 1.0f);
        }

        if (is_debugging)
        {
            SDL_SetStringProperty(info.props, "SDL.gpu.texture.create.name", path);
        }

        texture = SDL_CreateGPUTexture(device, &info);
        if (!texture)
        {
            error("Failed to create texture: %s", SDL_GetError());
            return NULL;
        }

        SDL_DestroyProperties(info.props);
    }

    {
        SDL_GPUTextureTransferInfo info = {0};
        info.transfer_buffer = transfer_buffer;

        SDL_GPUTextureRegion region = {0};
        region.texture = texture;
        region.w = width;
        region.h = height;
        region.d = 1;

        SDL_UploadToGPUTexture(copy_pass, &info, &region, false);
    }

    return texture;
}