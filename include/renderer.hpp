#pragma once

enum ShaderId
{
    shader_model_frag,
    shader_model_vert,
    shader_count,
};

enum GraphicsPipelineId
{
    graphics_pipeline_model,
    graphics_pipeline_count,
};

enum ComputePipelineId
{
    compute_pipeline_sampler,
    compute_pipeline_count,
};

enum SamplerId
{
    sampler_nearest,
    sampler_linear,
    sampler_count,
};

enum AttachmentId
{
    attachment_color,
    attachment_depth,
    attachment_count,
};

enum ModelId
{
    model_grass,
    model_count,
};

bool init_renderer(bool debug);

void shutdown_renderer();

void render();