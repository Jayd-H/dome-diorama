#include "Light.h"

LightBuilder::LightBuilder() {
  light.type = LightType::Point;
  light.name = "Unnamed Light";
  light.position = glm::vec3(0.0f);
  light.direction = glm::vec3(0.0f, -1.0f, 0.0f);
  light.color = glm::vec3(1.0f);
  light.intensity = 1.0f;
  light.constant = 1.0f;
  light.linear = 0.09f;
  light.quadratic = 0.032f;
  light.cutOff = 12.5f;
  light.outerCutOff = 17.5f;
}

LightBuilder& LightBuilder::type(LightType t) {
  light.type = t;
  return *this;
}

LightBuilder& LightBuilder::name(const std::string& n) {
  light.name = n;
  return *this;
}

LightBuilder& LightBuilder::position(float x, float y, float z) {
  light.position = glm::vec3(x, y, z);
  return *this;
}

LightBuilder& LightBuilder::position(const glm::vec3& pos) {
  light.position = pos;
  return *this;
}

LightBuilder& LightBuilder::direction(float x, float y, float z) {
  light.direction = glm::normalize(glm::vec3(x, y, z));
  return *this;
}

LightBuilder& LightBuilder::direction(const glm::vec3& dir) {
  light.direction = glm::normalize(dir);
  return *this;
}

LightBuilder& LightBuilder::color(float r, float g, float b) {
  light.color = glm::vec3(r, g, b);
  return *this;
}

LightBuilder& LightBuilder::color(const glm::vec3& c) {
  light.color = c;
  return *this;
}

LightBuilder& LightBuilder::intensity(float i) {
  light.intensity = i;
  return *this;
}

LightBuilder& LightBuilder::attenuation(float constant, float linear,
                                        float quadratic) {
  light.constant = constant;
  light.linear = linear;
  light.quadratic = quadratic;
  return *this;
}

LightBuilder& LightBuilder::spotAngles(float inner, float outer) {
  light.cutOff = inner;
  light.outerCutOff = outer;
  return *this;
}

Light LightBuilder::build() { return light; }

Light::Light() {
  type = LightType::Point;
  position = glm::vec3(0.0f);
  direction = glm::vec3(0.0f, -1.0f, 0.0f);
  color = glm::vec3(1.0f);
  intensity = 1.0f;
  constant = 1.0f;
  linear = 0.09f;
  quadratic = 0.032f;
  cutOff = 12.5f;
  outerCutOff = 17.5f;
  name = "Unnamed Light";
}