#pragma once
#include <glm/glm.hpp>
#include <string>

using LightID = uint32_t;
constexpr LightID INVALID_LIGHT_ID = 0;

enum class LightType { Point, Directional, Spot };

class Light {
 public:
  LightType type;
  glm::vec3 position;
  glm::vec3 direction;
  glm::vec3 color;
  float intensity;

  float constant;
  float linear;
  float quadratic;

  float cutOff;
  float outerCutOff;

  std::string name;

  Light();
};

class LightBuilder {
 public:
  LightBuilder();

  LightBuilder& type(LightType type);
  LightBuilder& name(const std::string& name);
  LightBuilder& position(const glm::vec3& pos);
  LightBuilder& position(float x, float y, float z);
  LightBuilder& direction(const glm::vec3& dir);
  LightBuilder& direction(float x, float y, float z);
  LightBuilder& color(const glm::vec3& col);
  LightBuilder& color(float r, float g, float b);
  LightBuilder& intensity(float intensity);
  LightBuilder& attenuation(float constant, float linear, float quadratic);
  LightBuilder& spotAngles(float cutOff, float outerCutOff);

  Light build();

 private:
  Light light;
};