#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "data.h"
#include "db.h"
#include "entity.h"
#include "gfx.h"
#include "level.h"
#include "math_ex.h"
#include "util.h"

#define render_target_pov_ratio 0.5f

typedef enum graphics_pipeline_type
{
    graphics_pipeline_type_pov,
    graphics_pipeline_type_composite,

    graphics_pipeline_type_count,
}
graphics_pipeline_type_t;

typedef enum render_target_type
{
    render_target_type_pov_color,
    render_target_type_pov_position,
    render_target_type_pov_normal,
    render_target_type_pov_depth,
    render_target_type_composite,

    render_target_type_count,
}
render_target_type_t;

typedef enum sampler_type
{
    sampler_type_nearest,
    sampler_type_linear,

    sampler_type_count,
}
sampler_type_t;

static SDL_Window* window;
static SDL_GPUDevice* device;

static SDL_GPUTextureFormat color_texture_format;
static SDL_GPUTextureFormat depth_texture_format;

static SDL_GPUShader* shaders[shader_type_count];
static SDL_GPUGraphicsPipeline* graphics_pipelines[graphics_pipeline_type_count];
static SDL_GPUComputePipeline* compute_pipelines[compute_pipeline_type_count];
static SDL_GPUTexture* render_targets[render_target_type_count];
static SDL_GPUSampler* samplers[sampler_type_count];

static mesh_t meshes[mesh_type_count];
static buffer_t instances[mesh_type_count];

static int swapchain_width; 
static int swapchain_height; 
static int pov_width;
static int pov_height;

static bool init_window()
{
    char title[256] = {0};
    if (is_debugging)
    {
        snprintf(title, sizeof(title), "lilcraft [debug]");
    }
    else
    {
        snprintf(title, sizeof(title), "lilcraft");
    }

    SDL_PropertiesID properties = SDL_GetGlobalProperties();

    SDL_SetStringProperty(properties, "SDL.window.create.title", title);
    SDL_SetNumberProperty(properties, "SDL.window.create.width", 960);
    SDL_SetNumberProperty(properties, "SDL.window.create.height", 720);
    SDL_SetBooleanProperty(properties, "SDL.window.create.resizable", true);

    window = SDL_CreateWindowWithProperties(properties);
    if (!window)
    {
        log_release("Failed to create window: %s", SDL_GetError());
        return false;
    }

    return true;
}

static bool init_device()
{
    SDL_PropertiesID properties = SDL_GetGlobalProperties();

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.spirv", true);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.dxil", true);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.msl", true);

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.debugmode", is_debugging);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.verbose", is_debugging);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.preferlowpower", true);

    device = SDL_CreateGPUDeviceWithProperties(properties);
    if (!device)
    {
        log_release("Failed to create device: %s", SDL_GetError());
        return false;
    }

    return true;
}

static bool init_swapchain()
{
    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        log_release("Failed to create swapchain");
        return false;
    }

    SDL_GPUTextureType type = SDL_GPU_TEXTURETYPE_2D;
    SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    color_texture_format = SDL_GetGPUSwapchainTextureFormat(device, window);
    if (SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, type, usage))
    {
        depth_texture_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    }
    else
    {
        depth_texture_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    }

    return true;
}

static bool init_shaders()
{
    for (shader_type_t i = 0; i < shader_type_count; i++)
    {
        shaders[i] = load_shader(device, get_shader_path(i));
        if (!shaders[i])
        {
            log_release("Failed to load shader: %d", i);
            return false;
        }
    }

    return true;
}

static bool init_compute_pipelines()
{
    for (compute_pipeline_type_t i = 0; i < compute_pipeline_type_count; i++)
    {
        compute_pipelines[i] = load_compute_pipeline(device, get_compute_pipeline_path(i));
        if (!compute_pipelines[i])
        {
            log_release("Failed to load compute pipeline: %d", i);
            return false;
        }
    }

    return true;
}

static bool init_graphics_pipelines()
{
    SDL_GPUGraphicsPipelineCreateInfo info[graphics_pipeline_type_count] =
    {
        [graphics_pipeline_type_pov] =
        {
            .vertex_shader = shaders[shader_type_mesh_vert],
            .fragment_shader = shaders[shader_type_mesh_frag],
            .target_info =
            {
                .num_color_targets = 3,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[])
                {{
                    .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
                },
                {
                    .format = color_texture_format,
                },
                {
                    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM,
                }},
            },
            .vertex_input_state =
            {
                .num_vertex_attributes = 4,
                .vertex_attributes = (SDL_GPUVertexAttribute[])
                {{
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
                    .location = 0,
                    .offset = offsetof(mesh_vertex_t, packed),
                },
                {
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
                    .location = 1,
                    .offset = offsetof(mesh_vertex_t, texcoord),
                },
                {
                    .buffer_slot = 1,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 2,
                    .offset = offsetof(transform_t, position),
                },
                {
                    .buffer_slot = 1,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
                    .location = 3,
                    .offset = offsetof(transform_t, rotation),
                }},
                .num_vertex_buffers = 2,
                .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
                {{
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    .instance_step_rate = 0,
                    .pitch = sizeof(mesh_vertex_t),
                    .slot = 0,
                },
                {
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                    .instance_step_rate = 0,
                    .pitch = sizeof(transform_t),
                    .slot = 1,
                }}
            },
            .depth_stencil_state =
            {
                .compare_op = SDL_GPU_COMPAREOP_LESS,
                .enable_depth_write = true,
                .enable_depth_test = true,
            },
            .rasterizer_state =
            {
                .fill_mode = SDL_GPU_FILLMODE_FILL,
                .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
                .cull_mode = SDL_GPU_CULLMODE_BACK,
            },
        },
        [graphics_pipeline_type_composite] =
        {
            .vertex_shader = shaders[shader_type_screen_vert],
            .fragment_shader = shaders[shader_type_composite_frag],
            .target_info =
            {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[])
                {{
                    .format = color_texture_format,
                }},
            },
        }
    };

    for (graphics_pipeline_type_t i = 0; i < graphics_pipeline_type_count; i++)
    {
        graphics_pipelines[i] = SDL_CreateGPUGraphicsPipeline(device, &info[i]);
        if (!graphics_pipelines[i])
        {
            log_release("Failed to create graphics pipeline: %d, %s", i, SDL_GetError());
            return false;
        }
    }

    return true;
}

static bool init_render_targets(int width, int height)
{
    assert_debug(width);
    assert_debug(height);

    if (swapchain_width == width && swapchain_height == height)
    {
        return true;
    }

    for (render_target_type_t i = 0; i < render_target_type_count; i++)
    {
        if (render_targets[i])
        {
            SDL_ReleaseGPUTexture(device, render_targets[i]);
            render_targets[i] = NULL;
        }
    }

    pov_width = width * render_target_pov_ratio;
    pov_height = height * render_target_pov_ratio;

    SDL_GPUTextureCreateInfo info = {0};
    if (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL)
    {
        info.props = SDL_GetGlobalProperties();
        SDL_SetFloatProperty(info.props, "SDL.gpu.texture.create.d3d12.clear.r", 0.0f);
        SDL_SetFloatProperty(info.props, "SDL.gpu.texture.create.d3d12.clear.g", 0.0f);
        SDL_SetFloatProperty(info.props, "SDL.gpu.texture.create.d3d12.clear.b", 0.0f);
        SDL_SetFloatProperty(info.props, "SDL.gpu.texture.create.d3d12.clear.a", 0.0f);
        SDL_SetFloatProperty(info.props, "SDL.gpu.texture.create.d3d12.clear.depth", 1.0f);
        SDL_SetNumberProperty(info.props, "SDL.gpu.texture.create.d3d12.clear.stencil", 0);
    }

    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.num_levels = 1;
    info.layer_count_or_depth = 1;

    info.width = pov_width;
    info.height = pov_height;

    info.format = color_texture_format;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    render_targets[render_target_type_pov_color] = SDL_CreateGPUTexture(device, &info);
    if (!render_targets[render_target_type_pov_color])
    {
        log_release("Failed to create pov color render target: %s", SDL_GetError());
        return false;
    }

    info.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    render_targets[render_target_type_pov_position] = SDL_CreateGPUTexture(device, &info);
    if (!render_targets[render_target_type_pov_position])
    {
        log_release("Failed to create pov position render target: %s", SDL_GetError());
        return false;
    }

    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    render_targets[render_target_type_pov_normal] = SDL_CreateGPUTexture(device, &info);
    if (!render_targets[render_target_type_pov_normal])
    {
        log_release("Failed to create pov normal render target: %s", SDL_GetError());
        return false;
    }

    info.format = depth_texture_format;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    render_targets[render_target_type_pov_depth] = SDL_CreateGPUTexture(device, &info);
    if (!render_targets[render_target_type_pov_depth])
    {
        log_release("Failed to create pov depth render target: %s", SDL_GetError());
        return false;
    }

    info.format = color_texture_format;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    render_targets[render_target_type_composite] = SDL_CreateGPUTexture(device, &info);
    if (!render_targets[render_target_type_composite])
    {
        log_release("Failed to create composite render target: %s", SDL_GetError());
        return false;
    }

    swapchain_width = width;
    swapchain_height = height;

    return true;
}

static bool init_samplers()
{
    SDL_GPUSamplerCreateInfo info = {0};
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    info.min_filter = SDL_GPU_FILTER_NEAREST;
    info.mag_filter = SDL_GPU_FILTER_NEAREST;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplers[sampler_type_nearest] = SDL_CreateGPUSampler(device, &info);
    if (!samplers[sampler_type_nearest])
    {
        log_release("Failed to create nearest sampler: %s", SDL_GetError());
        return false;
    }

    info.min_filter = SDL_GPU_FILTER_LINEAR;
    info.mag_filter = SDL_GPU_FILTER_LINEAR;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplers[sampler_type_linear] = SDL_CreateGPUSampler(device, &info);
    if (!samplers[sampler_type_linear])
    {
        log_release("Failed to create linear sampler: %s", SDL_GetError());
        return false;
    }

    return true;
}

static bool init_meshes()
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        log_release("Failed to acquire command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        log_release("Failed to begin copy pass: %s", SDL_GetError());
        return false;
    }

    for (mesh_type_t i = 0; i < mesh_type_count; i++)
    {
        if (!mesh_load(&meshes[i], device, copy_pass, get_mesh_path(i)))
        {
            log_release("Failed to load mesh: %d", i);
            return false;
        }
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return true;
}

static void draw_pov(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture)
{
    SDL_GPUColorTargetInfo color_info[3] = {0};
    SDL_GPUDepthStencilTargetInfo depth_info = {0};

    color_info[0].texture = render_targets[render_target_type_pov_color];
    color_info[0].load_op = SDL_GPU_LOADOP_CLEAR;
    color_info[0].store_op = SDL_GPU_STOREOP_STORE;
    color_info[0].cycle = true;

    color_info[1].texture = render_targets[render_target_type_pov_position];
    color_info[1].load_op = SDL_GPU_LOADOP_CLEAR;
    color_info[1].store_op = SDL_GPU_STOREOP_STORE;
    color_info[1].cycle = true;

    color_info[2].texture = render_targets[render_target_type_pov_normal];
    color_info[2].load_op = SDL_GPU_LOADOP_CLEAR;
    color_info[2].store_op = SDL_GPU_STOREOP_STORE;
    color_info[2].cycle = true;

    depth_info.texture = render_targets[render_target_type_pov_depth];
    depth_info.clear_depth = 1.0f;
    depth_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_info.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
    depth_info.store_op = SDL_GPU_STOREOP_STORE;
    depth_info.cycle = true;

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, color_info, 3, &depth_info);
    if (!render_pass)
    {
        log_release("Failed to begin render pass: %s", SDL_GetError());
        return;
    }

    SDL_BindGPUGraphicsPipeline(render_pass, graphics_pipelines[graphics_pipeline_type_pov]);

    /* TODO: matrix */

    for (mesh_type_t i = 0; i < mesh_type_count; i++)
    {
        if (!instances[i].size)
        {
            continue;
        }

        /* TODO: mesh/instance */
    }

    SDL_EndGPURenderPass(render_pass);
}

static void draw_frame()
{
    SDL_WaitForGPUSwapchain(device, window);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        log_release("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchain_texture;
    uint32_t width;
    uint32_t height;
    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &width, &height))
    {
        log_release("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    if (!swapchain_texture)
    {
        log_release("Invalid swapchain texture: %p", swapchain_texture);
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    if (!width || !height)
    {
        log_release("Invalid swapchain size: %d, %d", width, height);
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    if (!init_render_targets(width, height))
    {
        log_release("Failed to initialize render targets");
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    push_debug_group(device, command_buffer, "pov");
    draw_pov(command_buffer, swapchain_texture);
    pop_debug_group(device, command_buffer);

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

int main(int argc, char** argv)
{
    SDL_SetAppMetadata("lilcraft", NULL, NULL);

    log_init();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        log_release("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!TTF_Init())
    {
        log_release("Failed to initialize SDL ttf: %s", SDL_GetError());
        return 1;
    }

    if (!init_window())
    {
        log_release("Failed to initialize window");
        return 1;
    }

    if (!init_device())
    {
        log_release("Failed to initialize device");
        return 1;
    }

    if (!init_swapchain())
    {
        log_release("Failed to initialize swapchain");
        return 1;
    }

    if (!init_shaders())
    {
        log_release("Failed to initialize shaders");
        return 1;
    }

    if (!init_graphics_pipelines())
    {
        log_release("Failed to initialize graphics pipelines");
        return 1;
    }

    for (shader_type_t i = 0; i < shader_type_count; i++)
    {
        SDL_ReleaseGPUShader(device, shaders[i]);
        shaders[i] = NULL;
    }

    if (!init_compute_pipelines())
    {
        log_release("Failed to initialize compute pipelines");
        return 1;
    }

    if (!init_samplers())
    {
        log_release("Failed to initialize samplers: %s", SDL_GetError());
        return 1;
    }

    if (!init_meshes())
    {
        log_release("Failed to initialize meshes: %s", SDL_GetError());
        return 1;
    }

    if (!db_init("lilcraft.sqlite3"))
    {
        log_release("Failed to initialize database");
        log_release("Playing without saves enabled");
    }

    db_header_t db_header;
    db_get_header(&db_header);

    if (!level_init(db_header.new_db))
    {
        log_release("Failed to initialize level");
        return 1;
    }

    uint64_t t1 = SDL_GetPerformanceCounter();
    uint64_t t2 = 0;

    bool running = true;
    while (running)
    {
        t2 = t1;
        t1 = SDL_GetPerformanceCounter();

        const float frequency = SDL_GetPerformanceFrequency();
        const float dt = (t1 - t2) / frequency;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            }
        }

        if (!running)
        {
            break;
        }

        draw_frame();
    }

    SDL_HideWindow(window);

    for (mesh_type_t i = 0; i < mesh_type_count; i++)
    {
        mesh_free(&meshes[i], device);
        buffer_free(&instances[i], device);
    }

    for (graphics_pipeline_type_t i = 0; i < graphics_pipeline_type_count; i++)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipelines[i]);
        graphics_pipelines[i] = NULL;
    }

    for (compute_pipeline_type_t i = 0; i < compute_pipeline_type_count; i++)
    {
        SDL_ReleaseGPUComputePipeline(device, compute_pipelines[i]);
        compute_pipelines[i] = NULL;
    }

    for (render_target_type_t i = 0; i < render_target_type_count; i++)
    {
        SDL_ReleaseGPUTexture(device, render_targets[i]);
        render_targets[i] = NULL;
    }

    for (sampler_type_t i = 0; i < sampler_type_count; i++)
    {
        SDL_ReleaseGPUSampler(device, samplers[i]);
        samplers[i] = NULL;
    }

    level_quit();
    db_quit();

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);

    log_quit();

    TTF_Quit();
    SDL_Quit();

    return 0;
}