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

#define pov_resolution 0.5f

typedef enum graphics_pipeline_type
{
    graphics_pipeline_type_pov_mesh,
    graphics_pipeline_type_pov_composite,

    graphics_pipeline_type_count,
}
graphics_pipeline_type_t;

typedef enum render_target_type
{
    render_target_type_pov_color,
    render_target_type_pov_position,
    render_target_type_pov_normal,
    render_target_type_pov_depth,
    render_target_type_pov_composite,

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

typedef enum camera_type
{
    camera_type_pov,

    camera_type_count,
}
camera_type_t;

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
static camera_t cameras[camera_type_count];

static SDL_GPUCommandBuffer* command_buffer;
static SDL_GPUTexture* swapchain_texture;

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
        [graphics_pipeline_type_pov_mesh] =
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
                .has_depth_stencil_target = true,
                .depth_stencil_format = depth_texture_format,
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
        [graphics_pipeline_type_pov_composite] =
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

    pov_width = width * pov_resolution;
    pov_height = height * pov_resolution;

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
    render_targets[render_target_type_pov_composite] = SDL_CreateGPUTexture(device, &info);
    if (!render_targets[render_target_type_pov_composite])
    {
        log_release("Failed to create pov composite render target: %s", SDL_GetError());
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
        buffer_init(&instances[i], SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(transform_t), get_mesh_path(i));
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

static void init_cameras()
{
    camera_init(&cameras[camera_type_pov], camera_mode_perspective);
    camera_set_pitch(&cameras[camera_type_pov], -45.0f);
    camera_set_distance(&cameras[camera_type_pov], 100.0f);
}

static void upload_instance_entity_callback(const entity_t* entity, void* data)
{
    mesh_type_t mesh_type = entity_get_mesh_type(entity);
    transform_t transform = entity_get_transform(entity);

    assert_debug(mesh_type >= 0);
    assert_debug(mesh_type < mesh_type_count);

    buffer_append(&instances[mesh_type], device, &transform);
}

static void upload_instance_tile_callback(const tile_t* tile, int x, int z, void* data)
{
    mesh_type_t mesh_type = tile_get_mesh_type(tile);
    transform_t transform = tile_get_transform(tile, x, z);

    assert_debug(mesh_type >= 0);
    assert_debug(mesh_type < mesh_type_count);
   
    buffer_append(&instances[mesh_type], device, &transform);
}

static void upload_instances(SDL_GPUCommandBuffer* command_buffer)
{
    /* TODO: camera */
    aabb_t aabb;
    aabb.min.x = -100.0f;
    aabb.min.z = -100.0f;
    aabb.max.x = 100.0f;
    aabb.max.z = 100.0f;

    level_select_entity(upload_instance_entity_callback, &aabb, NULL);
    level_select_tile(upload_instance_tile_callback, &aabb, NULL);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        log_release("Failed to acquire copy pass: %s", SDL_GetError());
        return;
    }

    for (mesh_type_t i = 0; i < mesh_type_count; i++)
    {
        buffer_upload(&instances[i], device, copy_pass);
    }

    SDL_EndGPUCopyPass(copy_pass);
}

static void draw_pov_mesh(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GPUColorTargetInfo color_info[3] = {0};
    SDL_GPUDepthStencilTargetInfo depth_info = {0};

    color_info[0].texture = render_targets[render_target_type_pov_position];
    color_info[0].load_op = SDL_GPU_LOADOP_CLEAR;
    color_info[0].store_op = SDL_GPU_STOREOP_STORE;
    color_info[0].cycle = true;

    color_info[1].texture = render_targets[render_target_type_pov_color];
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
        log_release("Failed to begin draw_pov_mesh render pass: %s", SDL_GetError());
        return;
    }

    SDL_BindGPUGraphicsPipeline(render_pass, graphics_pipelines[graphics_pipeline_type_pov_mesh]);
    SDL_PushGPUVertexUniformData(command_buffer, 0, cameras[camera_type_pov].matrix, 64);

    for (mesh_type_t i = 0; i < mesh_type_count; i++)
    {
        if (!instances[i].size)
        {
            continue;
        }

        SDL_GPUBufferBinding vertex_buffers[2] = {0};
        vertex_buffers[0].buffer = meshes[i].vertex_buffer;
        vertex_buffers[1].buffer = instances[i].buffer;

        SDL_GPUBufferBinding index_buffer = {0};
        index_buffer.buffer = meshes[i].index_buffer;

        SDL_GPUTextureSamplerBinding palette = {0};
        palette.sampler = samplers[sampler_type_nearest];
        palette.texture = meshes[i].palette;

        SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buffers, 2);
        SDL_BindGPUIndexBuffer(render_pass, &index_buffer, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUFragmentSamplers(render_pass, 0, &palette, 1);

        SDL_DrawGPUIndexedPrimitives(render_pass, meshes[i].num_indices, instances[i].size, 0, 0, 0);
    }

    SDL_EndGPURenderPass(render_pass);
}

static void draw_pov_composite(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GPUColorTargetInfo color_info = {0};
    color_info.texture = render_targets[render_target_type_pov_composite];
    color_info.load_op = SDL_GPU_LOADOP_DONT_CARE;
    color_info.store_op = SDL_GPU_STOREOP_STORE;
    color_info.cycle = true;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(command_buffer, &color_info, 1, NULL);
    if (!pass)
    {
        log_release("Failed to begin draw_pov_composite render pass: %s", SDL_GetError());
        return;
    }

    SDL_GPUTextureSamplerBinding textures[3] = {0};

    textures[0].sampler = samplers[sampler_type_nearest];
    textures[1].sampler = samplers[sampler_type_nearest];
    textures[2].sampler = samplers[sampler_type_nearest];

    textures[0].texture = render_targets[render_target_type_pov_position];
    textures[1].texture = render_targets[render_target_type_pov_color];
    textures[2].texture = render_targets[render_target_type_pov_normal];

    SDL_BindGPUGraphicsPipeline(pass, graphics_pipelines[graphics_pipeline_type_pov_composite]);
    SDL_BindGPUFragmentSamplers(pass, 0, textures, 3);
    SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);

    SDL_EndGPURenderPass(pass);
}

static void draw_blit_pov(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture)
{
    SDL_GPUBlitInfo info = {0};

    info.filter = SDL_GPU_FILTER_NEAREST;

    info.source.texture = render_targets[render_target_type_pov_composite];
    info.source.x = 0;
    info.source.y = 0;
    info.source.w = pov_width;
    info.source.h = pov_height;

    info.destination.texture = swapchain_texture;
    info.destination.x = 0;
    info.destination.y = 0;
    info.destination.w = swapchain_width;
    info.destination.h = swapchain_height;

    info.load_op = SDL_GPU_LOADOP_DONT_CARE;

    SDL_BlitGPUTexture(command_buffer, &info);
}

static bool begin_frame()
{
    SDL_WaitForGPUSwapchain(device, window);

    command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        log_release("Failed to acquire command buffer: %s", SDL_GetError());
        return false;
    }

    uint32_t width;
    uint32_t height;

    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &width, &height))
    {
        log_release("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return false;
    }

    if (!swapchain_texture || !width || !height)
    {
        log_release("Invalid swapchain: %p, %d, %d", swapchain_texture, width, height);
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return false;
    }

    if (!init_render_targets(width, height))
    {
        log_release("Failed to initialize render targets");
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return false;
    }

    camera_set_width(&cameras[camera_type_pov], pov_width);
    camera_set_height(&cameras[camera_type_pov], pov_height);

    return true;
}

static void end_frame()
{
    push_debug_group(device, command_buffer, "upload_instances");
    upload_instances(command_buffer);
    pop_debug_group(device, command_buffer);

    push_debug_group(device, command_buffer, "draw_pov_mesh");
    draw_pov_mesh(command_buffer);
    pop_debug_group(device, command_buffer);

    push_debug_group(device, command_buffer, "draw_pov_composite");
    draw_pov_composite(command_buffer);
    pop_debug_group(device, command_buffer);

    push_debug_group(device, command_buffer, "draw_blit_pov");
    draw_blit_pov(command_buffer, swapchain_texture);
    pop_debug_group(device, command_buffer);

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

int main(int argc, char** argv)
{
    SDL_SetAppMetadata("lilcraft", NULL, NULL);

    log_init();
    init_cameras();

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

        if (!begin_frame())
        {
            continue;
        }

        /* TODO: camera */
        aabb_t aabb;
        aabb.min.x = -100.0f;
        aabb.min.z = -100.0f;
        aabb.max.x = 100.0f;
        aabb.max.z = 100.0f;

        camera_update(&cameras[camera_type_pov], 0.0f, 0.0f, dt, true);

        level_tick(dt, &aabb);

        end_frame();
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