#pragma once

#include <SDL3/SDL.h>

SDL_GPUShader* load_gpu_shader(SDL_GPUDevice* device, const char* name);

SDL_GPUComputePipeline* load_gpu_compute_pipeline(SDL_GPUDevice* device, const char* name);

SDL_GPUTexture* load_gpu_texture(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* path);