#pragma once

#include <SDL3/SDL.h>

class gpu_obj_model
{
public:
    bool load(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass, const char* name);
    void free(SDL_GPUDevice* device);

    SDL_GPUBuffer* get_vertex_buffer();
    SDL_GPUBuffer* get_index_buffer();
    SDL_GPUTexture* get_palette();

    int get_num_indices() const;

private:
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;
    SDL_GPUTexture* palette;

    int num_indices;
};