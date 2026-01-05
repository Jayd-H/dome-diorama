#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <string>

#include "Resources/Material.h"

using EmitterID = uint32_t;
constexpr EmitterID INVALID_EMITTER_ID = 0xFFFFFFFF;

struct ParticleInstanceData {
  float particleIndex;
  float padding1;
  float padding2;
  float padding3;
};

struct ParticleShaderParams {
  // Match GLSL layout exactly - vec3 followed by float for proper std140 packing
  alignas(16) glm::vec3 emitterPosition;
  alignas(4) float time;
  alignas(16) glm::vec3 baseColor;
  alignas(4) float particleLifetime;
  alignas(16) glm::vec3 tipColor;
  alignas(4) float maxParticles;
  alignas(16) glm::vec3 gravity;
  alignas(4) float spawnRadius;
  alignas(16) glm::vec3 initialVelocity;
  alignas(4) float particleScale;
  alignas(4) float fadeInDuration;
  alignas(4) float fadeOutDuration;
  alignas(4) int billboardMode;
  alignas(4) int colorMode;
  alignas(4) float velocityRandomness;
  alignas(4) float scaleOverLifetime;
  alignas(4) float rotationSpeed;
  alignas(4) float padding1;
  alignas(16) glm::vec3 windDirection;
  alignas(4) float windStrength;
  alignas(4) float windInfluence;
  alignas(4) float padding2;
  alignas(4) float padding3;
  alignas(4) float padding4;
};

class ParticleEmitter final {
 public:
  enum class BillboardMode { Spherical = 0, Cylindrical = 1, None = 2 };
  enum class ColorMode { Gradient = 0, BaseOnly = 1, TipOnly = 2 };

  ParticleEmitter();
  ~ParticleEmitter();

  ParticleEmitter(const ParticleEmitter&) = delete;
  ParticleEmitter& operator=(const ParticleEmitter&) = delete;

  void update(float deltaTime);

  void setPosition(const glm::vec3& pos);
  void setActive(bool isActive);
  void setWind(const glm::vec3& direction, float strength);
  void setName(const std::string& n);
  void setMaxParticles(size_t count);
  void setParticleLifetime(float lifetime);
  void setMaterialID(MaterialID id);

  void setBaseColor(const glm::vec3& color);
  void setTipColor(const glm::vec3& color);
  void setGravity(const glm::vec3& g);
  void setInitialVelocity(const glm::vec3& v);
  void setSpawnRadius(float radius);
  void setParticleScale(float scale);
  void setFadeInDuration(float duration);
  void setFadeOutDuration(float duration);
  void setBillboardMode(BillboardMode mode);
  void setColorMode(ColorMode mode);
  void setVelocityRandomness(float randomness);
  void setScaleOverLifetime(float scale);
  void setRotationSpeed(float speed);
  void setWindInfluence(float influence);

  void getName(std::string& outName) const;
  void getPosition(glm::vec3& outPos) const;
  bool isActive() const;
  size_t getMaxParticles() const;
  MaterialID getMaterialID() const;
  void getShaderParams(ParticleShaderParams& outParams) const;

 private:
  ParticleShaderParams shaderParams;
  std::string name;
  glm::vec3 position;
  size_t maxParticles;
  float particleLifetime;
  MaterialID materialID;
  float time;
  bool active;
};

inline void ParticleEmitter::update(float deltaTime) {
  if (!active) return;
  time += deltaTime;
  shaderParams.time = time;
  shaderParams.emitterPosition = position;
}

inline void ParticleEmitter::setPosition(const glm::vec3& pos) {
  position = pos;
  shaderParams.emitterPosition = pos;
}

inline void ParticleEmitter::setActive(bool isActive) { active = isActive; }
inline void ParticleEmitter::setName(const std::string& n) { name = n; }
inline void ParticleEmitter::setMaxParticles(size_t count) {
  maxParticles = count;
  shaderParams.maxParticles = static_cast<float>(count);
}
inline void ParticleEmitter::setParticleLifetime(float lifetime) {
  particleLifetime = lifetime;
  shaderParams.particleLifetime = lifetime;
}
inline void ParticleEmitter::setMaterialID(MaterialID id) { materialID = id; }
inline void ParticleEmitter::setBaseColor(const glm::vec3& color) {
  shaderParams.baseColor = color;
}
inline void ParticleEmitter::setTipColor(const glm::vec3& color) {
  shaderParams.tipColor = color;
}
inline void ParticleEmitter::setGravity(const glm::vec3& g) {
  shaderParams.gravity = g;
}
inline void ParticleEmitter::setInitialVelocity(const glm::vec3& v) {
  shaderParams.initialVelocity = v;
}
inline void ParticleEmitter::setSpawnRadius(float radius) {
  shaderParams.spawnRadius = radius;
}
inline void ParticleEmitter::setParticleScale(float scale) {
  shaderParams.particleScale = scale;
}
inline void ParticleEmitter::setFadeInDuration(float duration) {
  shaderParams.fadeInDuration = duration;
}
inline void ParticleEmitter::setFadeOutDuration(float duration) {
  shaderParams.fadeOutDuration = duration;
}
inline void ParticleEmitter::setBillboardMode(BillboardMode mode) {
  shaderParams.billboardMode = static_cast<int>(mode);
}
inline void ParticleEmitter::setColorMode(ColorMode mode) {
  shaderParams.colorMode = static_cast<int>(mode);
}
inline void ParticleEmitter::setVelocityRandomness(float randomness) {
  shaderParams.velocityRandomness = randomness;
}
inline void ParticleEmitter::setScaleOverLifetime(float scale) {
  shaderParams.scaleOverLifetime = scale;
}
inline void ParticleEmitter::setRotationSpeed(float speed) {
  shaderParams.rotationSpeed = speed;
}
inline void ParticleEmitter::setWindInfluence(float influence) {
  shaderParams.windInfluence = influence;
}
inline void ParticleEmitter::setWind(const glm::vec3& direction,
                                     float strength) {
  shaderParams.windDirection = direction;
  shaderParams.windStrength = strength;
}

inline void ParticleEmitter::getName(std::string& outName) const {
  outName = name;
}
inline void ParticleEmitter::getPosition(glm::vec3& outPos) const {
  outPos = position;
}
inline bool ParticleEmitter::isActive() const { return active; }
inline size_t ParticleEmitter::getMaxParticles() const { return maxParticles; }
inline MaterialID ParticleEmitter::getMaterialID() const { return materialID; }
inline void ParticleEmitter::getShaderParams(
    ParticleShaderParams& outParams) const {
  outParams = shaderParams;
}