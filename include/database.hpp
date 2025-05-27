#pragma once

#include <functional>
#include <memory>

enum LevelId;
class Entity;

namespace Database
{

bool init();
void shutdown();
bool has_save();
void commit();
void insert(std::shared_ptr<Entity>& entity, LevelId level);
void select(const std::function<void(std::shared_ptr<Entity>&, LevelId)>& callback);

}