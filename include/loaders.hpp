#pragma once

#include <SDL3/SDL.h>

#include <string>

SDL_GPUShader* load_shader(SDL_GPUDevice* device, const std::string& name);
SDL_GPUComputePipeline* load_compute_pipeline(SDL_GPUDevice* device, const std::string& name);
SDL_GPUTexture* load_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const std::string& path);