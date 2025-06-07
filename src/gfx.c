#include <SDL3/SDL.h>
#include <fast_obj.h>
#include <jsmn.h>
#include <stb_ds.h>
#include <stb_image.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "util.h"

SDL_GPUShader* load_shader(SDL_GPUDevice* device, const char* name)
{
    /* TODO: fix leaks on error */

    assert_debug(device);
    assert_debug(name);

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
        assert_release(false);
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
        log_release("Failed to load shader: %s", shader_path);
        return NULL;
    }

    char* shader_json_data = SDL_LoadFile(shader_json_path, &shader_json_size);
    if (!shader_json_data)
    {
        log_release("Failed to load shader json: %s", shader_json_path);
        return NULL;
    }

    jsmn_parser json_parser;
    jsmntok_t json_tokens[9];

    jsmn_init(&json_parser);
    if (jsmn_parse(&json_parser, shader_json_data, shader_json_size, json_tokens, 9) <= 0)
    {
        log_release("Failed to parse json: %s", shader_json_path);
        return NULL;
    }

    SDL_GPUShaderCreateInfo info = {0};

    for (int i = 1; i < 9; i += 2)
    {
        if (json_tokens[i].type != JSMN_STRING)
        {
            log_release("Bad json type: %s", shader_json_path);
            return NULL;
        }

        char* key_string = shader_json_data + json_tokens[i + 0].start;
        char* value_string = shader_json_data + json_tokens[i + 1].start;
        int key_size = json_tokens[i + 0].end - json_tokens[i + 0].start;

        uint32_t* value;

        if (!memcmp("samplers", key_string, key_size))
        {
            value = &info.num_samplers;
        }
        else if (!memcmp("storage_textures", key_string, key_size))
        {
            value = &info.num_storage_textures;
        }
        else if (!memcmp("storage_buffers", key_string, key_size))
        {
            value = &info.num_storage_buffers;
        }
        else if (!memcmp("uniform_buffers", key_string, key_size))
        {
            value = &info.num_uniform_buffers;
        }
        else
        {
            assert_release(false);
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
        if (!info.props)
        {
            log_release("Failed to create properties: %s", SDL_GetError());
            return NULL;
        }

        SDL_SetStringProperty(info.props, "SDL.gpu.shader.create.name", name);
    }

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    if (!shader)
    {
        log_release("Failed to create shader: %s", SDL_GetError());
        return NULL;
    }
        
    SDL_DestroyProperties(info.props);

    SDL_free(shader_data);
    SDL_free(shader_json_data);

    return shader;
}

SDL_GPUComputePipeline* load_compute_pipeline(SDL_GPUDevice* device, const char* name)
{
    /* TODO: fix leaks on error */

    assert_debug(device);
    assert_debug(name);

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
        assert_release(false);
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
        log_release("Failed to load shader: %s", shader_path);
        return NULL;
    }

    char* shader_json_data = SDL_LoadFile(shader_json_path, &shader_json_size);
    if (!shader_json_data)
    {
        log_release("Failed to load shader json: %s", shader_json_path);
        return NULL;
    }

    jsmn_parser json_parser;
    jsmntok_t json_tokens[19];

    jsmn_init(&json_parser);
    if (jsmn_parse(&json_parser, shader_json_data, shader_json_size, json_tokens, 19) <= 0)
    {
        log_release("Failed to parse json: %s", shader_json_path);
        return NULL;
    }

    SDL_GPUComputePipelineCreateInfo info = {0};

    for (int i = 1; i < 19; i += 2)
    {
        if (json_tokens[i].type != JSMN_STRING)
        {
            log_release("Bad json type: %s", shader_json_path);
            return NULL;
        }

        char* key_string = shader_json_data + json_tokens[i + 0].start;
        char* value_string = shader_json_data + json_tokens[i + 1].start;
        int key_size = json_tokens[i + 0].end - json_tokens[i + 0].start;

        uint32_t* value;

        if (!memcmp("samplers", key_string, key_size))
        {
            value = &info.num_samplers;
        }
        else if (!memcmp("readonly_storage_textures", key_string, key_size))
        {
            value = &info.num_readonly_storage_textures;
        }
        else if (!memcmp("readonly_storage_buffers", key_string, key_size))
        {
            value = &info.num_readonly_storage_buffers;
        }
        else if (!memcmp("readwrite_storage_textures", key_string, key_size))
        {
            value = &info.num_readwrite_storage_textures;
        }
        else if (!memcmp("readwrite_storage_buffers", key_string, key_size))
        {
            value = &info.num_readwrite_storage_buffers;
        }
        else if (!memcmp("uniform_buffers", key_string, key_size))
        {
            value = &info.num_uniform_buffers;
        }
        else if (!memcmp("threadcount_x", key_string, key_size))
        {
            value = &info.threadcount_x;
        }
        else if (!memcmp("threadcount_y", key_string, key_size))
        {
            value = &info.threadcount_y;
        }
        else if (!memcmp("threadcount_z", key_string, key_size))
        {
            value = &info.threadcount_z;
        }
        else
        {
            assert_release(false);
        }

        *value = *value_string - '0';
    }

    info.code = shader_data;
    info.code_size = shader_size;

    info.entrypoint = entrypoint;
    info.format = format;

    if (is_debugging)
    {
        info.props = SDL_CreateProperties();
        if (!info.props)
        {
            log_release("Failed to create properties: %s", SDL_GetError());
            return NULL;
        }

        SDL_SetStringProperty(info.props, "SDL.gpu.computepipeline.create.name", name);
    }

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &info);
    if (!pipeline)
    {
        log_release("Failed to create compute pipeline: %s", SDL_GetError());
        return NULL;
    }
        
    SDL_DestroyProperties(info.props);

    SDL_free(shader_data);
    SDL_free(shader_json_data);

    return pipeline;
}

SDL_GPUTexture* load_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path)
{
    /* TODO: fix leaks on error */

    assert_debug(device);
    assert_debug(copy_pass);
    assert_debug(path);

    int channels;
    int width;
    int height;

    void* src_data = stbi_load(path, &width, &height, &channels, 4);
    if (!src_data)
    {
        log_release("Failed to load image: %s, %s", path, stbi_failure_reason());
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
            log_release("Failed to create transfer buffer: %s", SDL_GetError());
            return NULL;
        }
    }

    void* dst_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!dst_data)
    {
        log_release("Failed to map transfer buffer: %s", SDL_GetError());
        return NULL;
    }

    memcpy(dst_data, src_data, width * height * 4);
    stbi_image_free(src_data);

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    {
        SDL_GPUTextureCreateInfo info = {0};

        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;

        info.num_levels = 1;

        if (is_debugging)
        {
            info.props = SDL_CreateProperties();
            if (!info.props)
            {
                log_release("Failed to create properties: %s", SDL_GetError());
                return NULL;
            }

            SDL_SetStringProperty(info.props, "SDL.gpu.texture.create.name", path);
        }

        texture = SDL_CreateGPUTexture(device, &info);
        if (!texture)
        {
            log_release("Failed to create texture: %s", SDL_GetError());
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

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    return texture;
}

void buffer_init(buffer_t* buffer, SDL_GPUBufferUsageFlags buffer_usage, uint32_t stride, const char* name)
{
    assert_debug(buffer);
    assert_debug(stride);

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
    assert_debug(buffer);
    assert_debug(device);

    assert_debug(!buffer->data);

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

void buffer_append(buffer_t* buffer, SDL_GPUDevice* device, const void* data)
{
    assert_debug(buffer);
    assert_debug(device);
    assert_debug(data);

    if (!buffer->data && buffer->transfer_buffer)
    {
        buffer->size = 0;
        buffer->resize = false;

        buffer->data = SDL_MapGPUTransferBuffer(device, buffer->transfer_buffer, true);
        if (!buffer->data)
        {
            log_release("Failed to map transfer buffer: %s, %s", buffer->name, SDL_GetError());
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
                log_release("Failed to create properties: %s, %s", buffer->name, SDL_GetError());
                return;
            }

            SDL_SetStringProperty(info.props, "SDL.gpu.transferbuffer.create.name", buffer->name);
        }

        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        SDL_DestroyProperties(info.props);
        if (!transfer_buffer)
        {
            log_release("Failed to create transfer buffer: %s, %s", buffer->name, SDL_GetError());
            return;
        }

        uint8_t* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        if (!data)
        {
            log_release("Failed to map transfer buffer: %s, %s", buffer->name, SDL_GetError());
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
    assert_debug(buffer);
    assert_debug(device);
    assert_debug(copy_pass);

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
                log_release("Failed to create properties: %s, %s", buffer->name, SDL_GetError());
                return;
            }

            SDL_SetStringProperty(info.props, "SDL.gpu.buffer.create.name", buffer->name);
        }

        buffer->buffer = SDL_CreateGPUBuffer(device, &info);
        SDL_DestroyProperties(info.props);
        if (!buffer->buffer)
        {
            log_release("Failed to create buffer: %s, %s", buffer->name, SDL_GetError());
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

bool mesh_load(mesh_t* mesh, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* name)
{
    /* TODO: fix leaks on error */

    assert_debug(mesh);
    assert_debug(device);
    assert_debug(copy_pass);
    assert_debug(name);

    SDL_PropertiesID properties = 0;
    if (is_debugging)
    {
        properties = SDL_CreateProperties();
        if (!properties)
        {
            log_release("Failed to create properties: %s", SDL_GetError());
            return false;
        }

        SDL_SetStringProperty(properties, "SDL.gpu.buffer.create.name", name);
        SDL_SetStringProperty(properties, "SDL.gpu.transferbuffer.create.name", name);
    }

    char obj_path[256] = {0};
    char png_path[256] = {0};

    snprintf(obj_path, sizeof(obj_path), "%s.obj", name);
    snprintf(png_path, sizeof(png_path), "%s.png", name);

    fastObjMesh* obj_mesh = fast_obj_read(obj_path);
    if (!obj_mesh)
    {
        log_release("Failed to load mesh: %s", obj_path);
        return false;
    }

    SDL_GPUTransferBuffer* vertex_buffer;
    SDL_GPUTransferBuffer* index_buffer;

    {
        SDL_GPUTransferBufferCreateInfo info = {0};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.props = properties;

        info.size = obj_mesh->index_count * sizeof(mesh_vertex_t);
        vertex_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!vertex_buffer)
        {
            log_release("Failed to create vertex transfer buffer: %s", SDL_GetError());
            return false;
        }

        info.size = obj_mesh->index_count * sizeof(mesh_index_t);
        index_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!index_buffer)
        {
            log_release("Failed to create index transfer buffer: %s", SDL_GetError());
            return false;
        }
    }

    mesh_vertex_t* vertex_data = SDL_MapGPUTransferBuffer(device, vertex_buffer, false);
    if (!vertex_data)
    {
        log_release("Failed to map vertex transfer buffer: %s", SDL_GetError());
        return false;
    }

    mesh_index_t* index_data = SDL_MapGPUTransferBuffer(device, index_buffer, false);
    if (!index_data)
    {
        log_release("Failed to map index transfer buffer: %s", SDL_GetError());
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
        log_release("Failed to create map");
        return false;
    }

    mesh->num_vertices = 0;
    mesh->num_indices = obj_mesh->index_count;

    for (uint32_t i = 0; i < obj_mesh->index_count; i++)
    {
        uint32_t position_index = obj_mesh->indices[i].p;
        uint32_t texcoord_index = obj_mesh->indices[i].t;
        uint32_t normal_index = obj_mesh->indices[i].n;

        if (position_index <= 0)
        {
            log_release("Missing position data: %s", name);
            return false;
        }

        if (texcoord_index <= 0)
        {
            log_release("Missing texcoord data: %s", name);
            return false;
        }

        if (normal_index <= 0)
        {
            log_release("Missing normal data: %s", name);
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

        assert_debug(position_x >= -16 && position_x <= 16);
        assert_debug(position_y >= -16 && position_y <= 16);
        assert_debug(position_z >= -16 && position_z <= 16);

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
            assert_debug(false);
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
        info.props = properties;

        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = mesh->num_vertices * sizeof(mesh_vertex_t);
        mesh->vertex_buffer = SDL_CreateGPUBuffer(device, &info);
        if (!mesh->vertex_buffer)
        {
            log_release("Failed to create vertex buffer: %s", SDL_GetError());
            return false;
        }

        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = mesh->num_indices * sizeof(mesh_index_t);
        mesh->index_buffer = SDL_CreateGPUBuffer(device, &info);
        if (!mesh->index_buffer)
        {
            log_release("Failed to create index buffer: %s", SDL_GetError());
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
        log_release("Failed to load palette: %s", png_path);
        return false;
    }

    SDL_DestroyProperties(properties);

    return true;
}

void mesh_free(mesh_t* mesh, SDL_GPUDevice* device)
{
    assert_debug(mesh);
    assert_debug(device);

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