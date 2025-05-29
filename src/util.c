#include <SDL3/SDL.h>
#include <stb_image.h>

#include <assert.h>
#include <stddef.h>

#include "util.h"

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