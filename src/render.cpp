#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <functional>
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
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;

    bool operator==(const model_vertex& other)
    {
        return
            position == other.position &&
            uv == other.uv &&
            normal == other.normal;
    }
};

struct packed_model_vertex
{
    packed_model_vertex(const model_vertex& vertex)
    {
        /* TODO: */
    }

    /*
     * 00-00: x sign bit
     * 01-07: x (7 bits)
     * 08-08: y sign bit
     * 09-15: y (7 bits)
     * 16-16: z sign bit
     * 17-23: z (7 bits)
     * 24-24: x normal
     * 25-25: y normal
     * 26-26: z normal
     * 27-31: unused (4 bits)
     */
    uint32_t packed;

    /*
     * x texcoord, y is hardcoded at 0.5
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
        size_t h1 = std::hash<glm::vec3>{}(vertex.position);
        size_t h2 = std::hash<glm::vec2>{}(vertex.uv);
        size_t h3 = std::hash<glm::vec3>{}(vertex.normal);
        return h1 ^ h2 ^ h3;
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

SDL_GPUShader* load_shader(const std::string name)
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

SDL_GPUComputePipeline* load_compute_pipeline(const std::string name)
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

SDL_GPUTexture* create_color_texture(uint32_t width, uint32_t height)
{
    return nullptr;
}

SDL_GPUTexture* create_position_texture(uint32_t width, uint32_t height)
{
    return nullptr;
}

SDL_GPUTexture* create_depth_texture(uint32_t width, uint32_t height)
{
    return nullptr;
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
        .offset = offsetof(packed_model_vertex, packed),
    },
    {
        .location = 1,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
        .offset = offsetof(packed_model_vertex, texcoord),
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
        .pitch = sizeof(packed_model_vertex),
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

    shader_format = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL;
    device = SDL_CreateGPUDevice(shader_format, true, nullptr);
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

std::shared_ptr<model> get_model(const std::string& string)
{
    return nullptr;
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

void draw(std::shared_ptr<model>& instance, const glm::vec3& position, float yaw)
{
    if (!has_frame)
    {
        return;
    }
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