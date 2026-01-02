#pragma once
#include <glm/glm.hpp>
#include <string>

using LightID = uint32_t;
constexpr LightID INVALID_LIGHT_ID = 0;

enum class LightType { Point, Directional, Spot };

class Light final {
 public:
  Light()
      : type(LightType::Point),
        position(0.0f),
        direction(0.0f, -1.0f, 0.0f),
        color(1.0f),
        intensity(1.0f),
        constant(1.0f),
        linear(0.09f),
        quadratic(0.032f),
        cutOff(12.5f),
        outerCutOff(17.5f),
        name("Unnamed Light"),
        castsShadows(false),
        shadowMapIndex(UINT32_MAX) {}

  Light(const Light&) = default;
  Light& operator=(const Light&) = default;

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
  bool castsShadows;
  uint32_t shadowMapIndex;
};

class LightBuilder final {
 public:
  inline LightBuilder() : light() {
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
    light.castsShadows = false;
    light.shadowMapIndex = UINT32_MAX;
  }

  inline LightBuilder& type(LightType t) {
    light.type = t;
    return *this;
  }

  inline LightBuilder& name(const std::string& n) {
    light.name = n;
    return *this;
  }

  inline LightBuilder& position(float x, float y, float z) {
    light.position = glm::vec3(x, y, z);
    return *this;
  }

  inline LightBuilder& position(const glm::vec3& pos) {
    light.position = pos;
    return *this;
  }

  inline LightBuilder& direction(float x, float y, float z) {
    light.direction = glm::normalize(glm::vec3(x, y, z));
    return *this;
  }

  inline LightBuilder& direction(const glm::vec3& dir) {
    light.direction = glm::normalize(dir);
    return *this;
  }

  inline LightBuilder& color(float r, float g, float b) {
    light.color = glm::vec3(r, g, b);
    return *this;
  }

  inline LightBuilder& color(const glm::vec3& c) {
    light.color = c;
    return *this;
  }

  inline LightBuilder& intensity(float i) {
    light.intensity = i;
    return *this;
  }

  inline LightBuilder& attenuation(float constant, float linear,
                                   float quadratic) {
    light.constant = constant;
    light.linear = linear;
    light.quadratic = quadratic;
    return *this;
  }

  inline LightBuilder& spotAngles(float inner, float outer) {
    light.cutOff = inner;
    light.outerCutOff = outer;
    return *this;
  }

  inline LightBuilder& castsShadows(bool shadows) {
    light.castsShadows = shadows;
    return *this;
  }

  inline Light build() const { return light; }

 private:
  Light light;
};