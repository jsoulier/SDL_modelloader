#pragma once

#include <SDL3/SDL.h>

class gpu_obj_model
{
public:
    bool load(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* name);
    void free(SDL_GPUDevice* device);

    SDL_GPUTexture* get_palette();
    SDL_GPUBuffer* get_vertex_buffer();
    SDL_GPUBuffer* get_index_buffer();

    int get_num_indices() const;

private:
    SDL_GPUTexture* palette;
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;

    int num_indices;
};