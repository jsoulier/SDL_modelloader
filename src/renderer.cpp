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

#include "camera.hpp"
#include "buffer.hpp"
#include "loaders.hpp"
#include "math.hpp"
#include "model.hpp"
#include "renderer.hpp"
#include "voxel.hpp"

enum ShaderId
{
    SHADER_MODEL_FRAG,
    SHADER_MODEL_VERT,
    SHADER_COUNT,
};

enum GraphicsPipelineId
{
    GRAPHICS_PIPELINE_MODEL,
    GRAPHICS_PIPELINE_COUNT,
};

enum ComputePipelineId
{
    COMPUTE_PIPELINE_SAMPLER,
    COMPUTE_PIPELINE_COUNT,
};

enum SamplerId
{
    SAMPLER_NEAREST,
    SAMPLER_LINEAR,
    SAMPLER_COUNT,
};

enum AttachmentId
{
    ATTACHMENT_COLOR,
    ATTACHMENT_DEPTH,
    ATTACHMENT_COUNT,
};

enum CameraId
{
    CAMERA_POV,
    CAMERA_COUNT,
};

static constexpr int WINDOW_WIDTH = 960;
static constexpr int WINDOW_HEIGHT = 720;
static constexpr int TARGET_WIDTH = 512;

static SDL_Window* window;
static SDL_GPUDevice* device;

static SDL_GPUTextureFormat color_texture_format;
static SDL_GPUTextureFormat depth_texture_format;

static std::array<SDL_GPUShader*, SHADER_COUNT> shaders;
static std::array<SDL_GPUGraphicsPipeline*, GRAPHICS_PIPELINE_COUNT> graphics_pipelines;
static std::array<SDL_GPUComputePipeline*, COMPUTE_PIPELINE_COUNT> compute_pipelines;
static std::array<SDL_GPUSampler*, SAMPLER_COUNT> samplers;
static std::array<SDL_GPUTexture*, ATTACHMENT_COUNT> attachments;

static std::array<Model, MODEL_COUNT> models;
static std::array<Buffer<Transform>, MODEL_COUNT> instances;

static std::array<Camera, CAMERA_COUNT> cameras;

static int swapchain_width;
static int swapchain_height;
static int texture_width;
static int texture_height;

static bool debug;

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
static void init_cameras();

static void free_shaders();
static void free_graphics_pipelines();
static void free_compute_pipelines();
static void free_samplers();
static void free_attachments();

static void upload_instances(SDL_GPUCommandBuffer* command_buffer);
static void render_pov(SDL_GPUCommandBuffer* command_buffer);
static void render_swapchain(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture);

static void push_debug_group(SDL_GPUCommandBuffer* command_buffer, const char* name);
static void pop_debug_group(SDL_GPUCommandBuffer* command_buffer);

bool Renderer::init(bool debug_requested)
{
    debug = debug_requested;

    SDL_PropertiesID properties = SDL_CreateProperties();
    if (!properties)
    {
        std::println("Failed to create properties: {}", SDL_GetError());
        return false;
    }

    SDL_SetStringProperty(properties, "SDL.window.create.title", "lilcraft");
    SDL_SetNumberProperty(properties, "SDL.window.create.width", WINDOW_WIDTH);
    SDL_SetNumberProperty(properties, "SDL.window.create.height", WINDOW_HEIGHT);
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
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.preferlowpower", true);

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

    init_cameras();

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return true;
}

void Renderer::shutdown()
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

void Renderer::wait()
{
    SDL_WaitForGPUSwapchain(device, window);
}

void Renderer::move(const Transform& transform)
{
    cameras[CAMERA_POV].set_target(transform.position);
}

const glm::vec2& Renderer::get_min()
{
    return cameras[CAMERA_POV].get_min();
}

const glm::vec2& Renderer::get_max()
{
    return cameras[CAMERA_POV].get_max();
}

void Renderer::draw(ModelId model, const Transform& transform)
{
    instances[model].push_back(device, transform);
}

void Renderer::submit(float dt)
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        std::println("Failed to acquire command buffer: {}", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchain_texture;
    uint32_t width;
    uint32_t height;

    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &width, &height))
    {
        std::println("Failed to acquire swapchain texture: {}", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    if (width == 0 || height == 0)
    {
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    if (width != swapchain_width || height != swapchain_height)
    {
        float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

        texture_width = TARGET_WIDTH;
        texture_height = static_cast<float>(TARGET_WIDTH) / aspect_ratio;

        free_attachments();
        if (init_attachments(texture_width, texture_height))
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

        cameras[CAMERA_POV].set_width(texture_width);
        cameras[CAMERA_POV].set_height(texture_height);
    }

    for (auto& camera : cameras)
    {
        camera.update(dt);
    }

    push_debug_group(command_buffer, "upload_instances");
    upload_instances(command_buffer);
    pop_debug_group(command_buffer);

    push_debug_group(command_buffer, "render_pov");
    render_pov(command_buffer);
    pop_debug_group(command_buffer);

    push_debug_group(command_buffer, "render_swapchain");
    render_swapchain(command_buffer, swapchain_texture);
    pop_debug_group(command_buffer);

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

static void push_debug_group(SDL_GPUCommandBuffer* command_buffer, const char* name)
{
    if (!debug)
    {
        return;
    }

    /* TODO: https://github.com/libsdl-org/SDL/issues/12056 */

    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        return;
    }

    SDL_PushGPUDebugGroup(command_buffer, name);
}

static void pop_debug_group(SDL_GPUCommandBuffer* command_buffer)
{
    if (!debug)
    {
        return;
    }

    /* TODO: https://github.com/libsdl-org/SDL/issues/12056 */

    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        return;
    }

    SDL_PopGPUDebugGroup(command_buffer);
}

static bool init_shaders()
{
    std::unordered_map<ShaderId, std::string> names =
    {
        {SHADER_MODEL_FRAG, "model.frag"},
        {SHADER_MODEL_VERT, "model.vert"},
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
        {GRAPHICS_PIPELINE_MODEL, create_model_graphics_pipeline},
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
        {COMPUTE_PIPELINE_SAMPLER, "sampler.comp"},
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
        {MODEL_DIRT, "dirt"},
        {MODEL_GRASS, "grass"},
        {MODEL_PLAYER, "player"},
        {MODEL_SAND, "sand"},
        {MODEL_TREE, "tree"},
        {MODEL_WATER, "water"},
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
        {SAMPLER_NEAREST, create_nearest_sampler},
        {SAMPLER_LINEAR, create_linear_sampler},
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
        {ATTACHMENT_COLOR, create_color_attachment},
        {ATTACHMENT_DEPTH, create_depth_attachment},
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

static void init_cameras()
{
    cameras[CAMERA_POV].set_type(CAMERA_PERSPECTIVE);
    cameras[CAMERA_POV].set_distance(150.0f);
    cameras[CAMERA_POV].set_fov(60.0f);
    cameras[CAMERA_POV].set_pitch(-60.0f);
    cameras[CAMERA_POV].set_yaw(-90.0f);
    cameras[CAMERA_POV].set_speed(0.1f);
}

static SDL_GPUGraphicsPipeline* create_model_graphics_pipeline()
{
    SDL_GPUGraphicsPipelineCreateInfo info{};

    info.vertex_shader = shaders[SHADER_MODEL_VERT];
    info.fragment_shader = shaders[SHADER_MODEL_FRAG];

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

static SDL_GPUTexture* create_depth_texture(int width, int height)
{
    SDL_PropertiesID properties = SDL_CreateProperties();
    if (!properties)
    {
        std::println("Failed to create properties");
        return nullptr;
    }

    SDL_SetFloatProperty(properties, "SDL.gpu.texture.create.d3d12.clear.depth", 1.0f);

    SDL_GPUTextureCreateInfo info{};

    info.props = properties;

    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = depth_texture_format;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;

    info.num_levels = 1;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &info);

    SDL_DestroyProperties(properties);

    return texture;
}

static SDL_GPUTexture* create_depth_attachment(int width, int height)
{
    return create_depth_texture(width, height);
}

static void free_shaders()
{
    for (int i = 0; i < SHADER_COUNT; i++)
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
    for (int i = 0; i < GRAPHICS_PIPELINE_COUNT; i++)
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
    for (int i = 0; i < COMPUTE_PIPELINE_COUNT; i++)
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
    for (int i = 0; i < SAMPLER_COUNT; i++)
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
    for (int i = 0; i < ATTACHMENT_COUNT; i++)
    {
        if (attachments[i])
        {
            SDL_ReleaseGPUTexture(device, attachments[i]);
            attachments[i] = nullptr;
        }
    }
}

static void upload_instances(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        std::println("Failed to begin copy pass: {}", SDL_GetError());
        return;
    }

    for (auto& instance : instances)
    {
        instance.upload(device, copy_pass);
    }

    SDL_EndGPUCopyPass(copy_pass);
}

static void render_pov(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GPUColorTargetInfo color_info{};
    color_info.texture = attachments[ATTACHMENT_COLOR];
    color_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_info.store_op = SDL_GPU_STOREOP_STORE;
    color_info.cycle = true;

    SDL_GPUDepthStencilTargetInfo depth_info{};
    depth_info.texture = attachments[ATTACHMENT_DEPTH];
    depth_info.clear_depth = 1.0f;
    depth_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_info.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
    depth_info.store_op = SDL_GPU_STOREOP_STORE;
    depth_info.cycle = true;

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_info, 1, &depth_info);
    if (!render_pass)
    {
        std::println("Failed to begin render pass: {}", SDL_GetError());
        return;
    }

    SDL_BindGPUGraphicsPipeline(render_pass, graphics_pipelines[GRAPHICS_PIPELINE_MODEL]);
    SDL_PushGPUVertexUniformData(command_buffer, 0, &cameras[CAMERA_POV].get_matrix(), 64);

    for (int i = 0; i < MODEL_COUNT; i++)
    {
        Model& model = models[i];
        Buffer<Transform>& instance = instances[i];

        if (!instance.get_size())
        {
            continue;
        }

        SDL_GPUBufferBinding vertex_buffers[2]{};
        vertex_buffers[0].buffer = model.get_vertex_buffer();
        vertex_buffers[1].buffer = instance.get_buffer();

        SDL_GPUBufferBinding index_buffer{};
        index_buffer.buffer = model.get_index_buffer();

        SDL_GPUTextureSamplerBinding texture{};
        texture.sampler = samplers[SAMPLER_NEAREST];
        texture.texture = model.get_palette();

        SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buffers, 2);
        SDL_BindGPUIndexBuffer(render_pass, &index_buffer, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUFragmentSamplers(render_pass, 0, &texture, 1);

        SDL_DrawGPUIndexedPrimitives(render_pass, model.get_num_indices(), instance.get_size(), 0, 0, 0);
    }

    SDL_EndGPURenderPass(render_pass);
}

static void render_swapchain(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture)
{
    SDL_GPUBlitInfo info{};

    info.filter = SDL_GPU_FILTER_NEAREST;

    info.source.texture = attachments[ATTACHMENT_COLOR];
    info.source.x = 0;
    info.source.y = 0;
    info.source.w = texture_width;
    info.source.h = texture_height;

    info.destination.texture = swapchain_texture;
    info.destination.x = 0;
    info.destination.y = 0;
    info.destination.w = swapchain_width;
    info.destination.h = swapchain_height;

    info.load_op = SDL_GPU_LOADOP_DONT_CARE;

    SDL_BlitGPUTexture(command_buffer, &info);
}