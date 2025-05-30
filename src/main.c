#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cglm/cglm.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "buffer.h"
#include "data.h"
#include "db.h"
#include "entity.h"
#include "mesh.h"
#include "util.h"

typedef enum shader
{
    shader_mesh_frag,
    shader_mesh_vert,
    shader_count,
}
shader_t;

typedef enum graphics_pipeline
{
    graphics_pipeline_pov,
    graphics_pipeline_count,
}
graphics_pipeline_t;

typedef enum compute_pipeline
{
    compute_pipeline_count,
}
compute_pipeline_t;

static SDL_IOStream* iostream;
static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_GPUTextureFormat color_texture_format;
static SDL_GPUTextureFormat depth_texture_format;
static SDL_GPUShader* shaders[shader_count];
static SDL_GPUGraphicsPipeline* graphics_pipelines[graphics_pipeline_count];
// static SDL_GPUComputePipeline* compute_pipelines[compute_pipeline_count];

static mesh_t meshes[mesh_type_count];
static buffer_t instances[mesh_type_count];

static void log_callback(void* data, int category, SDL_LogPriority priority, const char* string)
{
    if (!iostream)
    {
        iostream = SDL_IOFromFile("lilcraft.log", "w");
        if (!iostream)
        {
            SDL_SetLogOutputFunction(SDL_GetDefaultLogOutputFunction(), NULL);
            error("Failed to open iostream: %s", SDL_GetError());
        }
    }

    if (is_debugging)
    {
        SDL_GetDefaultLogOutputFunction()(data, category, priority, string);
    }

    if (iostream)
    {
        SDL_IOprintf(iostream, "%s\n", string);
    }
}

static bool init_window()
{
    SDL_PropertiesID properties = SDL_CreateProperties();
    if (!properties)
    {
        error("Failed to create properties: %s", SDL_GetError());
        return false;
    }

    SDL_SetStringProperty(properties, "SDL.window.create.title", "lilcraft");
    SDL_SetNumberProperty(properties, "SDL.window.create.width", 960);
    SDL_SetNumberProperty(properties, "SDL.window.create.height", 720);
    SDL_SetBooleanProperty(properties, "SDL.window.create.resizable", true);

    window = SDL_CreateWindowWithProperties(properties);
    if (!window)
    {
        error("Failed to create window: %s", SDL_GetError());
        return false;
    }

    SDL_DestroyProperties(properties);

    return true;
}

static bool init_device()
{
    SDL_PropertiesID properties = SDL_CreateProperties();
    if (!properties)
    {
        error("Failed to create properties: %s", SDL_GetError());
        return false;
    }

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.spirv", true);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.dxil", true);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.msl", true);

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.debugmode", is_debugging);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.verbose", is_debugging);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.preferlowpower", true);

    device = SDL_CreateGPUDeviceWithProperties(properties);
    if (!device)
    {
        error("Failed to create device: %s", SDL_GetError());
        return false;
    }

    SDL_DestroyProperties(properties);

    return true;
}

static bool init_swapchain()
{
    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        error("Failed to create swapchain");
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

    if (!SDL_SetGPUAllowedFramesInFlight(device, 1))
    {
        error("Failed to set frames in flight: %s", SDL_GetError());
        return false;
    }

    return true;
}

static bool init_shaders()
{
    static const char* names[shader_count] =
    {
        [shader_mesh_vert] = "mesh.vert",
        [shader_mesh_frag] = "mesh.frag",
    };

    for (shader_t i = 0; i < shader_count; i++)
    {
        shaders[i] = load_shader(device, names[i]);
        if (!shaders[i])
        {
            error("Failed to load shader: %s", names[i]);
            return false;
        }
    }

    return true;
}

static bool init_pov_graphics_pipeline(SDL_PropertiesID properties)
{
    if (is_debugging)
    {
        SDL_SetStringProperty(properties, "SDL.gpu.graphicspipeline.create.name", "pov");
    }

    SDL_GPUGraphicsPipelineCreateInfo info =
    {
        .vertex_shader = shaders[shader_mesh_vert],
        .fragment_shader = shaders[shader_mesh_frag],
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
        .props = properties,
    };

    graphics_pipelines[graphics_pipeline_pov] = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!graphics_pipelines[graphics_pipeline_pov])
    {
        return false;
    }

    return true;
}

static bool init_graphics_pipelines()
{
    SDL_PropertiesID properties = 0;
    if (is_debugging)
    {
        properties = SDL_CreateProperties();
        if (!properties)
        {
            error("Failed to create properties: %s", SDL_GetError());
            return false;
        }
    }

    if (!init_pov_graphics_pipeline(properties))
    {
        error("Failed to create graphics pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_DestroyProperties(properties);

    return true;
}

static bool init_compute_pipelines()
{
    return true;
}

static bool init_models()
{
    static const char* names[mesh_type_count] =
    {
        [mesh_type_dirt_00] = "dirt_00",
        [mesh_type_grass_00] = "grass_00",
        [mesh_type_player_00] = "player_00",
        [mesh_type_sand_00] = "sand_00",
        [mesh_type_tree_00] = "tree_00",
        [mesh_type_water_00] = "water_00",
    };

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        error("Failed to acquire command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        error("Failed to begin copy pass: %s", SDL_GetError());
        return false;
    }

    for (mesh_type_t i = 0; i < mesh_type_count; i++)
    {
        if (!load_mesh(&meshes[i], device, copy_pass, names[i]))
        {
            error("Failed to load mesh: %s", names[i]);
            return false;
        }
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return true;
}

static void draw_pov()
{

}

static void draw_frame()
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        error("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchain_texture;
    uint32_t width;
    uint32_t height;

    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &width, &height))
    {
        error("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return;
    }

    if (width == 0 || height == 0)
    {
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    /* TODO: resize render targets */

    push_debug_group(device, command_buffer, "pov");
    draw_pov();
    pop_debug_group(device, command_buffer);

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

int main(int argc, char** argv)
{
    SDL_SetAppMetadata("lilcraft", NULL, NULL);

    if (is_debugging)
    {
        SDL_SetLogOutputFunction(log_callback, NULL);
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        error("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!init_window())
    {
        error("Failed to initialize window");
        return 1;
    }

    if (!init_device())
    {
        error("Failed to initialize device");
        return 1;
    }

    if (!init_swapchain(window))
    {
        error("Failed to initialize swapchain");
        return 1;
    }

    if (!init_shaders())
    {
        error("Failed to initialize shaders");
        return 1;
    }

    if (!init_graphics_pipelines())
    {
        error("Failed to initialize graphics pipelines");
        return 1;
    }

    if (!init_compute_pipelines())
    {
        error("Failed to initialize compute pipelines");
        return 1;
    }

    for (shader_t i = 0; i < shader_count; i++)
    {
        SDL_ReleaseGPUShader(device, shaders[i]);
        shaders[i] = NULL;
    }

    if (!db_init("lilcraft.sqlite3"))
    {
        error("Failed to initialize database");
        warning("Playing without saves enabled");
    }

    bool running = true;
    while (running)
    {
        SDL_WaitForGPUSwapchain(device, window);

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

    db_quit();

    for (mesh_type_t i = 0; i < mesh_type_count; i++)
    {
        mesh_free(&meshes[i], device);
        buffer_free(&instances[i], device);
    }

    for (graphics_pipeline_t i = 0; i < graphics_pipeline_count; i++)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipelines[i]);
        graphics_pipelines[i] = NULL;
    }

    // for (compute_pipeline_t i = 0; i < compute_pipeline_count; i++)
    // {
    //     SDL_ReleaseGPUComputePipeline(device, compute_pipelines[i]);
    //     compute_pipelines[i] = NULL;
    // }

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    SDL_CloseIO(iostream);

    return 0;
}