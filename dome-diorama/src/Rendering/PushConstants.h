#pragma once
#include <glm/glm.hpp>

struct PlantPushConstants {
  alignas(16) glm::mat4 model;
  alignas(16) glm::vec3 windDirection;
  alignas(4) float windStrength;
  alignas(4) float time;
  alignas(4) float swayAmount;
  alignas(4) float swaySpeed;
  alignas(4) float isPlant;
};

struct StandardPushConstants {
  alignas(16) glm::mat4 model;
};