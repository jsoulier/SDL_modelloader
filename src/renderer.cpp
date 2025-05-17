#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <print>
#include <string>
#include <unordered_map>
#include <utility>

#include "buffer.hpp"
#include "loaders.hpp"
#include "math.hpp"
#include "model.hpp"
#include "renderer.hpp"
#include "voxel.hpp"

static constexpr int window_width = 960;
static constexpr int window_height = 720;
static constexpr int target_width = 512;

static SDL_Window* window;
static SDL_GPUDevice* device;

static SDL_GPUTextureFormat color_texture_format;
static SDL_GPUTextureFormat depth_texture_format;

static std::array<SDL_GPUShader*, shader_count> shaders;
static std::array<SDL_GPUGraphicsPipeline*, graphics_pipeline_count> graphics_pipelines;
static std::array<SDL_GPUComputePipeline*, compute_pipeline_count> compute_pipelines;
static std::array<SDL_GPUSampler*, sampler_count> samplers;
static std::array<SDL_GPUTexture*, attachment_count> attachments;

static std::array<Model, model_count> models;
static std::array<Buffer<Transform>, model_count> instances;

static int swapchain_width;
static int swapchain_height;
static int texture_width;
static int texture_height;

static SDL_GPUGraphicsPipeline* create_model_graphics_pipeline();

static SDL_GPUSampler* create_nearest_sampler();
static SDL_GPUSampler* create_linear_sampler();

static SDL_GPUTexture* create_color_attachment(int width, int height);
static SDL_GPUTexture* create_depth_attachment(int width, int height);

static bool init_shaders();
static bool init_graphics_pipelines();
static bool init_compute_pipelines();
static bool init_models(SDL_GPUCopyPass* copy_pass);
static bool init_samplers();
static bool init_attachments(int width, int height);

static void free_shaders();
static void free_graphics_pipelines();
static void free_compute_pipelines();
static void free_samplers();
static void free_attachments();

bool init_renderer(bool debug)
{
    SDL_PropertiesID properties = SDL_CreateProperties();
    if (!properties)
    {
        std::println("Failed to create properties: {}", SDL_GetError());
        return false;
    }

    SDL_SetStringProperty(properties, "SDL.window.create.title", "lilcraft");
    SDL_SetNumberProperty(properties, "SDL.window.create.width", window_width);
    SDL_SetNumberProperty(properties, "SDL.window.create.height", window_height);
    SDL_SetBooleanProperty(properties, "SDL.window.create.resizable", true);

    window = SDL_CreateWindowWithProperties(properties);
    if (!window)
    {
        std::println("Failed to create window: {}", SDL_GetError());
        return false;
    }

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.spirv", true);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.dxil", true);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.msl", true);

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.debugmode", debug);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.verbose", debug);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.preferlowpower", !debug);

    device = SDL_CreateGPUDeviceWithProperties(properties);
    if (!device)
    {
        std::println("Failed to create device: {}", SDL_GetError());
        return false;
    }

    SDL_DestroyProperties(properties);

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        std::println("Failed to create swapchain: {}", SDL_GetError());
        return false;
    }

    color_texture_format = SDL_GetGPUSwapchainTextureFormat(device, window);
    depth_texture_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        std::println("Failed to acquire command buffer: {}", SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        std::println("Failed to copy pass: {}", SDL_GetError());
        return false;
    }

    if (!init_shaders())
    {
        std::println("Failed to create shaders");
        return false;
    }

    if (!init_graphics_pipelines())
    {
        std::println("Failed to create graphics pipelines: {}", SDL_GetError());
        return false;
    }

    if (!init_compute_pipelines())
    {
        std::println("Failed to create compute pipelines");
        return false;
    }

    if (!init_samplers())
    {
        std::println("Failed to create samplers: {}", SDL_GetError());
        return false;
    }

    if (!init_models(copy_pass))
    {
        std::println("Failed to create models");
        return false;
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return true;
}

void shutdown_renderer()
{
    free_shaders();
    free_graphics_pipelines();
    free_compute_pipelines();
    free_samplers();
    free_attachments();

    for (auto& model : models)
    {
        model.free(device);
    }

    for (auto& instance : instances)
    {
        instance.free(device);
    }

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
}

void render()
{
    SDL_WaitForGPUSwapchain(device, window);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        std::println("Failed to acquire command buffer: {}", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchain_texture;
    uint32_t width;
    uint32_t height;

    if (SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &width, &height))
    {
        std::println("Failed to acquire swapchain texture: {}", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    if (width != swapchain_width || height != swapchain_height)
    {
        float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

        texture_width = target_width;
        texture_height = static_cast<float>(target_width) / aspect_ratio;

        free_attachments();
        if (init_attachments(width, height))
        {
            swapchain_width = width;
            swapchain_height = height;
        }
        else
        {
            std::println("Failed to create attachments: {}", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(command_buffer);
            return;
        }
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

static bool init_shaders()
{
    std::unordered_map<ShaderId, std::string> names =
    {
        {shader_model_frag, "model.frag"},
        {shader_model_vert, "model.vert"},
    };

    for (auto& [shader, name] : names)
    {
        shaders[shader] = load_shader(device, name);
        if (!shaders[shader])
        {
            return false;
        }
    }

    return true;
}

static bool init_graphics_pipelines()
{
    std::unordered_map<GraphicsPipelineId, std::function<SDL_GPUGraphicsPipeline*()>> funcs =
    {
        {graphics_pipeline_model, create_model_graphics_pipeline},
    };

    for (auto& [graphics_pipeline, func] : funcs)
    {
        graphics_pipelines[graphics_pipeline] = func();
        if (!graphics_pipelines[graphics_pipeline])
        {
            return false;
        }
    }

    return true;
}

static bool init_compute_pipelines()
{
    std::unordered_map<ComputePipelineId, std::string> names =
    {
        {compute_pipeline_sampler, "sampler.comp"},
    };

    for (auto& [compute_pipeline, name] : names)
    {
        compute_pipelines[compute_pipeline] = load_compute_pipeline(device, name);
        if (!compute_pipelines[compute_pipeline])
        {
            return false;
        }
    }

    return true;
}

static bool init_models(SDL_GPUCopyPass* copy_pass)
{
    std::unordered_map<ModelId, std::string> names =
    {
        {model_grass, "grass"},
    };

    for (auto& [model, name] : names)
    {
        bool result = models[model].load(device, copy_pass, name);
        if (!result)
        {
            return false;
        }
    }

    return true;
}

static bool init_samplers()
{
    std::unordered_map<SamplerId, std::function<SDL_GPUSampler*()>> funcs =
    {
        {sampler_nearest, create_nearest_sampler},
        {sampler_linear, create_linear_sampler},
    };

    for (auto& [sampler, func] : funcs)
    {
        samplers[sampler] = func();
        if (!samplers[sampler])
        {
            return false;
        }
    }

    return true;
}

static bool init_attachments(int width, int height)
{
    std::unordered_map<AttachmentId, std::function<SDL_GPUTexture*(int, int)>> funcs =
    {
        {attachment_color, create_color_attachment},
        {attachment_depth, create_depth_attachment},
    };

    for (auto& [attachment, func] : funcs)
    {
        attachments[attachment] = func(width, height);
        if (!attachments[attachment])
        {
            return false;
        }
    }

    return true;
}

static SDL_GPUGraphicsPipeline* create_model_graphics_pipeline()
{
    SDL_GPUGraphicsPipelineCreateInfo info{};

    info.vertex_shader = shaders[shader_model_vert];
    info.fragment_shader = shaders[shader_model_frag];

    SDL_GPUColorTargetDescription targets[] =
    {{
        .format = color_texture_format,
    }};

    info.target_info =
    {
        .color_target_descriptions = targets,
        .num_color_targets = 1,
        .depth_stencil_format = depth_texture_format,
        .has_depth_stencil_target = true,
    };

    SDL_GPUVertexAttribute attributes[] =
    {{
        .location = 0,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
        .offset = offsetof(Voxel, packed),
    },
    {
        .location = 1,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
        .offset = offsetof(Voxel, texcoord),
    },
    {
        .location = 2,
        .buffer_slot = 1,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = offsetof(Transform, position),
    },
    {
        .location = 3,
        .buffer_slot = 1,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
        .offset = offsetof(Transform, rotation),
    }};

    SDL_GPUVertexBufferDescription buffers[] =
    {{
        .slot = 0,
        .pitch = sizeof(Voxel),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    },
    {
        .slot = 1,
        .pitch = sizeof(Transform),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        .instance_step_rate = 0,
    }};

    info.vertex_input_state =
    {
        .vertex_buffer_descriptions = buffers,
        .num_vertex_buffers = 2,
        .vertex_attributes = attributes,
        .num_vertex_attributes = 4,
    };

    info.depth_stencil_state =
    {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    info.rasterizer_state =
    {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_BACK,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
    };

    return SDL_CreateGPUGraphicsPipeline(device, &info);
}

static SDL_GPUSampler* create_nearest_sampler()
{
    SDL_GPUSamplerCreateInfo info{};

    info.min_filter = SDL_GPU_FILTER_NEAREST;
    info.mag_filter = SDL_GPU_FILTER_NEAREST;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;

    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    return SDL_CreateGPUSampler(device, &info);
}

static SDL_GPUSampler* create_linear_sampler()
{
    SDL_GPUSamplerCreateInfo info{};

    info.min_filter = SDL_GPU_FILTER_LINEAR;
    info.mag_filter = SDL_GPU_FILTER_LINEAR;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;

    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    return SDL_CreateGPUSampler(device, &info);
}

static SDL_GPUTexture* create_color_attachment(int width, int height)
{
    SDL_GPUTextureCreateInfo info{};

    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = color_texture_format;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    return SDL_CreateGPUTexture(device, &info);
}

static SDL_GPUTexture* create_depth_attachment(int width, int height)
{
    SDL_GPUTextureCreateInfo info{};

    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = depth_texture_format;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    return SDL_CreateGPUTexture(device, &info);
}

static void free_shaders()
{
    for (int i = 0; i < shader_count; i++)
    {
        if (shaders[i])
        {
            SDL_ReleaseGPUShader(device, shaders[i]);
            shaders[i] = nullptr;
        }
    }
}

static void free_graphics_pipelines()
{
    for (int i = 0; i < graphics_pipeline_count; i++)
    {
        if (graphics_pipelines[i])
        {
            SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipelines[i]);
            graphics_pipelines[i] = nullptr;
        }
    }
}

static void free_compute_pipelines()
{
    for (int i = 0; i < compute_pipeline_count; i++)
    {
        if (compute_pipelines[i])
        {
            SDL_ReleaseGPUComputePipeline(device, compute_pipelines[i]);
            compute_pipelines[i] = nullptr;
        }
    }
}

static void free_samplers()
{
    for (int i = 0; i < sampler_count; i++)
    {
        if (samplers[i])
        {
            SDL_ReleaseGPUSampler(device, samplers[i]);
            samplers[i] = nullptr;
        }
    }
}

static void free_attachments()
{
    for (int i = 0; i < attachment_count; i++)
    {
        if (attachments[i])
        {
            SDL_ReleaseGPUTexture(device, attachments[i]);
            attachments[i] = nullptr;
        }
    }
}