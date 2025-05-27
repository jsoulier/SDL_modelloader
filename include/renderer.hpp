#pragma once

#include "math.hpp"

enum ModelId
{
    MODEL_DIRT,
    MODEL_GRASS,
    MODEL_PLAYER,
    MODEL_SAND,
    MODEL_TREE,
    MODEL_WATER,
    MODEL_COUNT,
};

namespace Renderer
{

bool init(bool debug);
void shutdown();
void wait();
void move(const Transform& transform);

const glm::vec2& get_min();
const glm::vec2& get_max();

void draw(ModelId model, const Transform& transform);
void submit(float dt);

}