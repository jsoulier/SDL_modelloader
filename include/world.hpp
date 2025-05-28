#pragma once

#include <memory>

class Entity;

enum LevelId
{
    /* don't change the order! */
    LEVEL_OVERWORLD,
    LEVEL_COUNT,
};

namespace World
{

bool init();
void shutdown();

std::shared_ptr<Entity> get_player();

void move();
void update(float dt);
void draw();
void commit();
void insert(std::shared_ptr<Entity>& entity, LevelId level = LEVEL_COUNT);

}