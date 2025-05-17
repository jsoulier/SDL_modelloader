#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

#include <print>

#include "components.hpp"
#include "dynamic_gpu_buffer.hpp"
#include "gpu_loaders.hpp"
#include "gpu_obj_model.hpp"
#include "graphics.hpp"
#include "packed_vertex.hpp"

static constexpr int window_width = 960;
static constexpr int window_height = 720;
static constexpr int target_width = 512;

static SDL_Window* window;
static SDL_GPUDevice* device;

static bool debugging;

bool graphics::init(bool debug)
{
    debugging = debug;

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

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.debugmode", debugging);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.verbose", debugging);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.preferlowpower", !debugging);

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

    if (debugging)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplSDLGPU3_InitInfo info{};
        info.Device = device;
        info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
        ImGui_ImplSDL3_InitForSDLGPU(window);
        ImGui_ImplSDLGPU3_Init(&info);
    }

    return true;
}

void graphics::quit()
{
    if (debugging)
    {
        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();
    }

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
}

static void draw_imgui(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* color_texture)
{
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    /* TODO: */

    ImGui::Render();

    ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), command_buffer);

    SDL_GPUColorTargetInfo info{};
    info.load_op = SDL_GPU_LOADOP_CLEAR; /* TODO: */
    info.store_op = SDL_GPU_STOREOP_STORE;
    info.texture = color_texture;

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &info, 1, nullptr);
    if (!render_pass)
    {
        std::println("Failed to begin render pass: {}", SDL_GetError());
        return;
    }

    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), command_buffer, render_pass);

    SDL_EndGPURenderPass(render_pass);
}

void graphics::draw()
{
    SDL_WaitForGPUSwapchain(device, window);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        std::println("Failed to acquire command buffer: {}", SDL_GetError());
        return;
    }

    SDL_GPUTexture* color_texture;
    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, window, &color_texture, nullptr, nullptr))
    {
        SDL_CancelGPUCommandBuffer(command_buffer);
        std::println("Failed to acquire swapchain texture: {}", SDL_GetError());
        return;
    }

    if (debugging)
    {
        SDL_PushGPUDebugGroup(command_buffer, "draw_imgui");
        draw_imgui(command_buffer, color_texture);
        SDL_PopGPUDebugGroup(command_buffer);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
}