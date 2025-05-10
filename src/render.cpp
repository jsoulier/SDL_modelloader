#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

#include "render.hpp"

namespace
{

enum texture
{
    texture_color,
    texture_position,
    texture_normal,
    texture_depth,
    texture_count,
};

#define SHADERS \
    X(model_frag) \
    X(model_vert) \

enum shader
{
#define X(name) shader_##name,
    SHADERS
#undef X
    shader_count,
};

#define GRAPHICS_PIPELINES \
    X(model) \

enum graphics_pipeline
{
#define X(name) graphics_pipeline_##name,
    GRAPHICS_PIPELINES
#undef X
    graphics_pipeline_count,
};

#define COMPUTE_PIPELINES \
    X(sampler) \

enum compute_pipeline
{
#define X(name) compute_pipeline_##name,
    COMPUTE_PIPELINES
#undef X
    compute_pipeline_count,
};

struct transform
{
    glm::vec3 position;
    float yaw;
};

struct model_vertex
{
    model_vertex(const glm::vec3& position, const glm::vec2& uv, const glm::vec3& normal)
    {
        packed = 0;
        texcoord = uv.x;

        assert(position.x >= -63.0f && position.x <= 63.0f);
        assert(position.y >= -63.0f && position.y <= 63.0f);
        assert(position.z >= -63.0f && position.z <= 63.0f);

        packed |= (std::abs(static_cast<int>(position.x)) & 0x7F) << 0;
        packed |= (std::signbit(position.x) ? 1 : 0) << 7;
        packed |= (std::abs(static_cast<int>(position.y)) & 0x7F) << 8;
        packed |= (std::signbit(position.y) ? 1 : 0) << 15;
        packed |= (std::abs(static_cast<int>(position.z)) & 0x7F) << 16;
        packed |= (std::signbit(position.z) ? 1 : 0) << 23;

        uint32_t direction = 0;
        if (normal.x > 0)
        {
            direction = 0;
        }
        else if (normal.x < 0)
        {
            direction = 1;
        }
        else if (normal.y > 0)
        {
            direction = 2;
        }
        else if (normal.y < 0)
        {
            direction = 3;
        }
        else if (normal.z > 0)
        {
            direction = 4;
        }
        else if (normal.z < 0)
        {
            direction = 5;
        }
        else
        {
            assert(false);
        }
        packed |= (direction & 0x7) << 24;
    }

    bool operator==(const model_vertex& other) const
    {
        static constexpr float epsilon = std::numeric_limits<float>::epsilon();
        return packed == other.packed && std::fabs(texcoord - other.texcoord) < epsilon;
    }

    /*
     * 00-00: x sign bit
     * 01-07: x (7 bits)
     * 08-08: y sign bit
     * 09-15: y (7 bits)
     * 16-16: z sign bit
     * 17-23: z (7 bits)
     * 24-26: direction (3 bits)
     * 27-31: unused (5 bits)
     */
    uint32_t packed;

    /*
     * x texcoord (y is hardcoded at 0.5 in the shader)
     */
    float texcoord;
};

} /* namespace */

namespace std
{

template<>
struct hash<model_vertex>
{
    size_t operator()(const model_vertex& vertex) const
    {
        size_t h1 = std::hash<uint32_t>{}(vertex.packed);
        size_t h2 = std::hash<float>{}(vertex.texcoord);
        return h1 ^ h2;
    }
};

} /* namespace std */

struct model
{
    bool loaded;
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;
    SDL_GPUTexture* palette;

    std::vector<transform> instances;
    SDL_GPUTransferBuffer* instance_transfer_buffer;
    SDL_GPUBuffer* instance_buffer;
    uint32_t instance_capacity;
};

namespace
{

static constexpr uint32_t render_width = 256;
static constexpr uint32_t render_height = 144;

SDL_Window* window;
SDL_GPUDevice* device;
TTF_TextEngine* text_engine;
SDL_GPUShaderFormat shader_format;
std::string shader_entrypoint;
std::string shader_suffix;
SDL_GPUTextureFormat color_texture_format;
SDL_GPUTextureFormat depth_texture_format;
std::array<SDL_GPUTexture*, texture_count> textures;
std::array<SDL_GPUShader*, shader_count> shaders;
std::array<SDL_GPUGraphicsPipeline*, graphics_pipeline_count> graphics_pipelines;
std::array<SDL_GPUComputePipeline*, compute_pipeline_count> compute_pipelines;
std::unordered_map<std::string, std::shared_ptr<model>> models;
SDL_GPUCommandBuffer* deferred_command_buffer;
SDL_GPUCommandBuffer* forward_command_buffer;
SDL_GPUTexture* swapchain_texture;
bool has_frame;

SDL_GPUShader* load_shader(const std::string& name)
{
    std::string shader_path = name + shader_suffix;
    std::string json_path = name + ".json";

    SDL_GPUShaderCreateInfo info = {0};

    void* code = SDL_LoadFile(shader_path.data(), &info.code_size);
    if (!code)
    {
        std::println("Failed to load shader: {}, {}", name, SDL_GetError());
        return nullptr;
    }
    info.code = static_cast<uint8_t*>(code);

    std::fstream file(json_path);
    if (!file.good())
    {
        std::println("Failed to open json: {}", name);
        return nullptr;
    }
    nlohmann::json json;
    try
    {
        json << file;
    }
    catch (const std::exception& e)
    {
        std::println("Failed to load json: {}, {}", name, e.what());
        return nullptr;
    }

    info.num_uniform_buffers = json["uniform_buffers"];
    info.num_samplers = json["samplers"];
    info.num_storage_buffers = json["storage_buffers"];
    info.num_storage_textures = json["storage_textures"];

    if (name.contains(".vert"))
    {
        info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else
    {
        info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }

    info.format = shader_format;
    info.entrypoint = shader_entrypoint.data();

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    if (!shader)
    {
        std::println("Failed to create shader: {}, {}", name, SDL_GetError());
        return nullptr;
    }

    SDL_free(code);
    return shader;
}

SDL_GPUComputePipeline* load_compute_pipeline(const std::string& name)
{
    std::string shader_path = name + ".comp" + shader_suffix;
    std::string json_path = name + ".comp" + ".json";

    SDL_GPUComputePipelineCreateInfo info = {0};

    void* code = SDL_LoadFile(shader_path.data(), &info.code_size);
    if (!code)
    {
        std::println("Failed to load shader: {}, {}", name, SDL_GetError());
        return nullptr;
    }
    info.code = static_cast<uint8_t*>(code);

    std::fstream file(json_path);
    if (!file.good())
    {
        std::println("Failed to open json: {}", name);
        return nullptr;
    }
    nlohmann::json json;
    try
    {
        json << file;
    }
    catch (const std::exception& e)
    {
        std::println("Failed to load json: {}, {}", name, e.what());
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
    info.format = shader_format;
    info.entrypoint = shader_entrypoint.data();

    SDL_GPUComputePipeline* shader = SDL_CreateGPUComputePipeline(device, &info);
    if (!shader)
    {
        std::println("Failed to create compute pipeline: {}, {}", name, SDL_GetError());
        return nullptr;
    }

    SDL_free(code);
    return shader;
}

SDL_GPUGraphicsPipeline* create_model_pipeline()
{
    SDL_GPUColorTargetDescription color_targets[] =
    {{
        .format = color_texture_format,
    },
    {
        .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
    },
    {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM,
    }};

    SDL_GPUVertexAttribute attributes[] =
    {{
        .location = 0,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
        .offset = offsetof(model_vertex, packed),
    },
    {
        .location = 1,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
        .offset = offsetof(model_vertex, texcoord),
    },
    {
        .location = 2,
        .buffer_slot = 1,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = offsetof(transform, position),
    },
    {
        .location = 3,
        .buffer_slot = 1,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = offsetof(transform, yaw),
    }};

    SDL_GPUVertexBufferDescription buffers[] =
    {{
        .slot = 0,
        .pitch = sizeof(model_vertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    },
    {
        .slot = 1,
        .pitch = sizeof(transform),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        .instance_step_rate = 0,
    }};

    SDL_GPUGraphicsPipelineCreateInfo info{};

    info.vertex_shader = shaders[shader_model_vert];
    info.fragment_shader = shaders[shader_model_frag];

    info.target_info.num_color_targets = 3;
    info.target_info.color_target_descriptions = color_targets;
    info.target_info.has_depth_stencil_target = true;
    info.target_info.depth_stencil_format = depth_texture_format,

    info.vertex_input_state.num_vertex_attributes = 4;
    info.vertex_input_state.vertex_attributes = attributes;
    info.vertex_input_state.num_vertex_buffers = 2;
    info.vertex_input_state.vertex_buffer_descriptions = buffers;

    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    return SDL_CreateGPUGraphicsPipeline(device, &info);
}

SDL_GPUTexture* load_texture(const std::string& path)
{
    int width;
    int height;
    int channels;

    void* src = stbi_load(path.data(), &width, &height, &channels, 4);
    if (!src)
    {
        std::println("Failed to load image: {}, {}", path, stbi_failure_reason());
        return nullptr;
    }

    SDL_GPUTextureCreateInfo info{};

    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &info);
    if (!texture)
    {
        std::println("Failed to create texture: {}, {}", path, SDL_GetError());
        stbi_image_free(src);
        return nullptr;
    }

    SDL_GPUTransferBufferCreateInfo tbci{};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size = width * height * 4;
    SDL_GPUTransferBuffer* buffer = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!buffer)
    {
        std::println("Failed to create transfer buffer: {}, {}", path, SDL_GetError());
        stbi_image_free(src);
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }

    void* dst = SDL_MapGPUTransferBuffer(device, buffer, false);
    if (!dst)
    {
        std::println("Failed to map transfer buffer: {}, {}", path, SDL_GetError());
        stbi_image_free(src);
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }

    memcpy(dst, src, width * height * 4);
    stbi_image_free(src);
    SDL_UnmapGPUTransferBuffer(device, buffer);

    SDL_GPUTextureTransferInfo transfer_info = {0};
    SDL_GPUTextureRegion region = {0};
    transfer_info.transfer_buffer = buffer;
    region.texture = texture;
    region.w = width;
    region.h = height;
    region.d = 1;

    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands)
    {
        std::println("Failed to acquire command buffer: {}, {}", path, SDL_GetError());
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }

    SDL_GPUCopyPass* copy  = SDL_BeginGPUCopyPass(commands);
    if (!copy)
    {
        std::println("Failed to begin copy pass: {}, {}", path, SDL_GetError());
        SDL_ReleaseGPUTexture(device, texture);
        SDL_CancelGPUCommandBuffer(commands);
        return nullptr;
    }

    SDL_UploadToGPUTexture(copy, &transfer_info, &region, true);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(commands);
    SDL_ReleaseGPUTransferBuffer(device, buffer);

    return texture;
}

SDL_GPUTexture* create_color_texture(uint32_t width, uint32_t height, SDL_GPUTextureUsageFlags usage = 0)
{
    SDL_GPUTextureCreateInfo info{};

    info.format = color_texture_format;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | usage;
    info.type = SDL_GPU_TEXTURETYPE_2D;

    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    return SDL_CreateGPUTexture(device, &info);
}

SDL_GPUTexture* create_position_texture(uint32_t width, uint32_t height, SDL_GPUTextureUsageFlags usage = 0)
{
    SDL_GPUTextureCreateInfo info{};

    info.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | usage;
    info.type = SDL_GPU_TEXTURETYPE_2D;

    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    return SDL_CreateGPUTexture(device, &info);
}

SDL_GPUTexture* create_normal_texture(uint32_t width, uint32_t height, SDL_GPUTextureUsageFlags usage = 0)
{
    SDL_GPUTextureCreateInfo info{};

    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | usage;
    info.type = SDL_GPU_TEXTURETYPE_2D;

    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    return SDL_CreateGPUTexture(device, &info);
}

SDL_GPUTexture* create_depth_texture(uint32_t width, uint32_t height, SDL_GPUTextureUsageFlags usage = 0)
{
    SDL_GPUTextureCreateInfo info{};

    info.format = depth_texture_format;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | usage;
    info.type = SDL_GPU_TEXTURETYPE_2D;

    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    return SDL_CreateGPUTexture(device, &info);
}

} /* namespace */

namespace render
{

bool init()
{
    window = SDL_CreateWindow("lilcraft", 960, 720, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::println("Failed to create window: {}", SDL_GetError());
        return false;
    }

#ifndef NDEBUG
    bool debug = true;
#else
    bool debug = false;
#endif

    shader_format = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL;
    device = SDL_CreateGPUDevice(shader_format, debug, nullptr);
    if (!device)
    {
        std::println("Failed to create device: {}", SDL_GetError());
        return false;
    }

    shader_format = SDL_GetGPUShaderFormats(device);
    if (shader_format & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        shader_format = SDL_GPU_SHADERFORMAT_SPIRV;
        shader_entrypoint = "main";
        shader_suffix = ".spv";
    }
    else if (shader_format & SDL_GPU_SHADERFORMAT_DXIL)
    {
        shader_format = SDL_GPU_SHADERFORMAT_DXIL;
        shader_entrypoint = "main";
        shader_suffix = ".dxil";
    }
    else if (shader_format & SDL_GPU_SHADERFORMAT_MSL)
    {
        shader_format = SDL_GPU_SHADERFORMAT_MSL;
        shader_entrypoint = "main0";
        shader_suffix = ".msl";
    }
    else
    {
        assert(false);
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        std::println("Failed to create swapchain: {}", SDL_GetError());
        return false;
    }

    color_texture_format = SDL_GetGPUSwapchainTextureFormat(device, window);
    depth_texture_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    /* TODO: parallelize */

    textures[texture_color] = create_color_texture(render_width, render_height,
        SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ);

    textures[texture_position] = create_color_texture(render_width, render_height,
        SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ);

    textures[texture_normal] = create_color_texture(render_width, render_height,
        SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ);

    textures[texture_depth] = create_depth_texture(render_width, render_height);

    for (int i = 0; i < texture_count; i++)
    {
        if (!textures[i])
        {
            std::println("Failed to create texture(s)");
            return false;
        }
    }

#define X(name) \
    std::string shader_##name##_str = #name; \
    shader_##name##_str.replace(shader_##name##_str.find('_'), 1, "."); \
    shaders[shader_##name] = load_shader(shader_##name##_str); \
    if (!shaders[shader_##name]) \
    { \
        std::println("Failed to load shader: {}", #name); \
        return false; \
    } \

    SHADERS
#undef X

#define X(name) \
    graphics_pipelines[graphics_pipeline_##name] = create_##name##_pipeline(); \
    if (!graphics_pipelines[graphics_pipeline_##name]) \
    { \
        std::println("Failed to load graphics pipeline: {}, {}", #name, SDL_GetError()); \
        return false; \
    }

    GRAPHICS_PIPELINES
#undef X

#define X(name) \
    compute_pipelines[compute_pipeline_##name] = load_compute_pipeline(#name); \
    if (!compute_pipelines[compute_pipeline_##name]) \
    { \
        std::println("Failed to load compute pipeline: {}, {}", #name, SDL_GetError()); \
        return false; \
    } \

    COMPUTE_PIPELINES
#undef X

    if (!TTF_Init())
    {
        std::println("Failed to initialize SDL ttf: {}", SDL_GetError());
        return false;
    }

    text_engine = TTF_CreateGPUTextEngine(device);
    if (!text_engine)
    {
        std::println("Failed to create text engine: {}", SDL_GetError());
        return false;
    }

    return true;
}

void quit()
{
    for (SDL_GPUTexture* texture : textures)
    {
        SDL_ReleaseGPUTexture(device, texture);
    }
    for (SDL_GPUShader* shader : shaders)
    {
        SDL_ReleaseGPUShader(device, shader);
    }
    for (SDL_GPUGraphicsPipeline* graphics_pipeline : graphics_pipelines)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipeline);
    }
    for (SDL_GPUComputePipeline* compute_pipeline : compute_pipelines)
    {
        SDL_ReleaseGPUComputePipeline(device, compute_pipeline);
    }

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);

    TTF_DestroyGPUTextEngine(text_engine);
    TTF_Quit();
}

std::shared_ptr<model> get_model(const std::string& name)
{
    /* TODO: lazy load */
    /* TODO: fix leaks on error */

    auto it = models.find(name);
    if (it != models.end())
    {
        return it->second;
    }

    std::string obj_path = name + ".obj";
    std::string png_path = name + ".png";

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(obj_path))
    {
        std::println("Failed to get model: {}, {}", name, reader.Error());
        return nullptr;
    }

    tinyobj::attrib_t attrib = reader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
    std::vector<uint32_t> indices;
    std::vector<model_vertex> vertices;
    std::unordered_map<model_vertex, uint32_t> unique_vertices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            glm::vec3 position;
            glm::vec2 uv;
            glm::vec3 normal;

            position.x = attrib.vertices[3 * index.vertex_index + 0];
            position.y = attrib.vertices[3 * index.vertex_index + 1];
            position.z = attrib.vertices[3 * index.vertex_index + 2];

            uv.x = attrib.texcoords[3 * index.texcoord_index + 0];
            uv.y = attrib.texcoords[2 * index.texcoord_index + 1];

            normal.x = attrib.normals[3 * index.normal_index + 0];
            normal.y = attrib.normals[3 * index.normal_index + 1];
            normal.z = attrib.normals[3 * index.normal_index + 2];

            model_vertex vertex{position, uv, normal};

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

    std::shared_ptr<model> m = std::make_shared<model>();
    if (!m)
    {
        std::println("Failed to allocate model");
        return nullptr;
    }

    models.emplace(name, m);

    m->palette = load_texture(png_path);
    if (!m->palette)
    {
        std::println("Failed to load palette");
        return m;
    }

    SDL_GPUCommandBuffer* command_buffer;
    SDL_GPUCopyPass* copy_pass;
    SDL_GPUTransferBuffer* vertex_transfer_buffer;
    SDL_GPUTransferBuffer* index_transfer_buffer;

    command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        std::println("Failed to acquire command buffer: {}", SDL_GetError());
        return m;
    }

    copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        std::println("Failed to begin copy pass: {}", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return m;
    }

    {
        SDL_GPUBufferCreateInfo info{};

        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = vertices.size() * sizeof(model_vertex);
        m->vertex_buffer = SDL_CreateGPUBuffer(device, &info);

        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = indices.size() * sizeof(uint32_t);
        m->index_buffer = SDL_CreateGPUBuffer(device, &info);

        if (!m->vertex_buffer || !m->index_buffer)
        {
            std::println("Failed to create buffer(s): {}", SDL_GetError());
            return m;
        }
    }

    {
        SDL_GPUTransferBufferCreateInfo info{};

        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = vertices.size() * sizeof(model_vertex);
        vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);

        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = indices.size() * sizeof(uint32_t);
        index_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);

        if (!vertex_transfer_buffer || !index_transfer_buffer)
        {
            std::println("Failed to create transfer buffer(s): {}", SDL_GetError());
            return m;
        }
    }

    void* vertex_data = SDL_MapGPUTransferBuffer(device, vertex_transfer_buffer, false);
    void* index_data = SDL_MapGPUTransferBuffer(device, index_transfer_buffer, false);
    if (!vertex_data || !index_data)
    {
        std::println("Failed to map buffer(s): {}", SDL_GetError());
        return m;
    }

    std::memcpy(vertex_data, vertices.data(), vertices.size() * sizeof(model_vertex));
    std::memcpy(index_data, indices.data(), indices.size() * sizeof(uint32_t));

    SDL_UnmapGPUTransferBuffer(device, vertex_transfer_buffer);
    SDL_UnmapGPUTransferBuffer(device, index_transfer_buffer);

    SDL_GPUBufferRegion region{};
    SDL_GPUTransferBufferLocation location{};

    region.buffer = m->vertex_buffer;
    region.size = vertices.size() * sizeof(model_vertex);
    location.transfer_buffer = vertex_transfer_buffer;
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

    region.buffer = m->index_buffer;
    region.size = indices.size() * sizeof(uint32_t);
    location.transfer_buffer = index_transfer_buffer;
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    m->loaded = true;
    return m;
}

void draw(std::shared_ptr<model>& m, const glm::vec3& position, float yaw)
{
    if (!has_frame)
    {
        return;
    }
}

void begin_frame()
{
    has_frame = false;

    SDL_WaitForGPUSwapchain(device, window);

    deferred_command_buffer = SDL_AcquireGPUCommandBuffer(device);
    forward_command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!deferred_command_buffer || !forward_command_buffer)
    {
        std::println("Failed to acquire command buffer(s): {}", SDL_GetError());
        return;
    }

    if (!SDL_AcquireGPUSwapchainTexture(deferred_command_buffer, window, &swapchain_texture, nullptr, nullptr))
    {
        std::println("Failed to acquire swapchain texture: {}", SDL_GetError());
        SDL_CancelGPUCommandBuffer(deferred_command_buffer);
        SDL_CancelGPUCommandBuffer(forward_command_buffer);
        return;
    }

    if (!swapchain_texture)
    {
        SDL_SubmitGPUCommandBuffer(deferred_command_buffer);
        SDL_SubmitGPUCommandBuffer(forward_command_buffer);
        return;
    }

    has_frame = true;
}

void end_deferred_frame()
{
    if (!has_frame)
    {
        return;
    }

    SDL_SubmitGPUCommandBuffer(deferred_command_buffer);
}

void end_forward_frame()
{
    if (!has_frame)
    {
        return;
    }

    SDL_SubmitGPUCommandBuffer(forward_command_buffer);
}

} /* namespace render */