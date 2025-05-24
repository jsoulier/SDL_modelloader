#pragma once

struct Transform;

enum ModelId
{
    model_dirt,
    model_grass,
    model_player,
    model_sand,
    model_tree,
    model_water,
    model_count,
};

bool init_renderer(bool debug);

void shutdown_renderer();

void wait_for_renderer();

void move_renderer(const Transform& transform);

void render_model(ModelId model, const Transform& transform);

void render_frame(float dt);