#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>

struct model;

namespace render
{

bool init();
void quit();
std::shared_ptr<model> get_model(const std::string& name);
void draw(std::shared_ptr<model>& m, const glm::vec3& position, float yaw);
void begin_frame();
void end_deferred_frame();
void end_forward_frame();

}