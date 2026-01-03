#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

#include "Resources/Material.h"

using EmitterID = uint32_t;
constexpr EmitterID INVALID_EMITTER_ID = 0;

struct ParticleInstanceData {
  float particleIndex;
  float padding1;
  float padding2;
  float padding3;
};

struct ParticleShaderParams {
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
  alignas(16) glm::vec3 windDirection;
  alignas(4) float windStrength;
  alignas(4) float windInfluence;
  alignas(4) float padding1;
  alignas(4) float padding2;
  alignas(4) float padding3;
};

class ParticleEmitter {
 public:
  enum class BillboardMode { Spherical = 0, Cylindrical = 1, None = 2 };

  enum class ColorMode { Gradient = 0, BaseOnly = 1, TipOnly = 2 };

  ParticleEmitter()
      : maxParticles(1000),
        particleLifetime(2.0f),
        materialID(INVALID_MATERIAL_ID),
        time(0.0f),
        shaderParams{},
        name("Unnamed Emitter"),
        position(0.0f),
        active(true),
        windInfluence(0.0f) {
    shaderParams.emitterPosition = position;
    shaderParams.time = 0.0f;
    shaderParams.particleLifetime = particleLifetime;
    shaderParams.maxParticles = static_cast<float>(maxParticles);
    shaderParams.gravity = glm::vec3(0.0f);
    shaderParams.spawnRadius = 1.0f;
    shaderParams.initialVelocity = glm::vec3(0.0f);
    shaderParams.particleScale = 1.0f;
    shaderParams.fadeInDuration = 0.1f;
    shaderParams.fadeOutDuration = 0.3f;
    shaderParams.billboardMode = static_cast<int>(BillboardMode::Spherical);
    shaderParams.colorMode = static_cast<int>(ColorMode::Gradient);
    shaderParams.velocityRandomness = 0.5f;
    shaderParams.scaleOverLifetime = 1.0f;
    shaderParams.rotationSpeed = 0.0f;
    shaderParams.windDirection = glm::vec3(0.0f);
    shaderParams.windStrength = 0.0f;
    shaderParams.windInfluence = 1.0f;
  }

  virtual ~ParticleEmitter() = default;

  virtual void update(float deltaTime) {
    if (!active) return;
    time += deltaTime;
    shaderParams.time = time;
    shaderParams.emitterPosition = position;
    updateShaderParams();
  }

  inline void setPosition(const glm::vec3& pos) {
    position = pos;
    shaderParams.emitterPosition = pos;
  }

  inline void setActive(bool isActive) { active = isActive; }

  inline void setWind(const glm::vec3& direction, float strength) {
    shaderParams.windDirection = direction;
    shaderParams.windStrength = strength;
  }

  inline void setWindInfluence(float influence) {
    windInfluence = influence;
    shaderParams.windInfluence = influence;
  }

  inline const std::string& getName() const { return name; }
  inline void setName(const std::string& n) { name = n; }

  inline const glm::vec3& getPosition() const { return position; }
  inline bool isActive() const { return active; }

  inline size_t getMaxParticles() const { return maxParticles; }
  inline MaterialID getMaterialID() const { return materialID; }
  inline ParticleShaderParams getShaderParams() const { return shaderParams; }

 protected:
  size_t maxParticles;
  float particleLifetime;
  MaterialID materialID;
  float time;
  ParticleShaderParams shaderParams;
  float windInfluence;

  virtual void updateShaderParams() = 0;

  friend class ParticleEmitterBuilder;
  friend class ParticleManager;

 private:
  std::string name;
  glm::vec3 position;
  bool active;
};

class ParticleEmitterBuilder {
 public:
  ParticleEmitterBuilder() : emitter(nullptr) {}

  virtual ~ParticleEmitterBuilder() = default;

  ParticleEmitterBuilder(const ParticleEmitterBuilder&) = delete;
  ParticleEmitterBuilder& operator=(const ParticleEmitterBuilder&) = delete;

  ParticleEmitterBuilder& name(const std::string& n) {
    if (emitter) emitter->name = n;
    return *this;
  }

  ParticleEmitterBuilder& position(const glm::vec3& pos) {
    if (emitter) {
      emitter->position = pos;
      emitter->shaderParams.emitterPosition = pos;
    }
    return *this;
  }

  ParticleEmitterBuilder& position(float x, float y, float z) {
    if (emitter) {
      emitter->position = glm::vec3(x, y, z);
      emitter->shaderParams.emitterPosition = glm::vec3(x, y, z);
    }
    return *this;
  }

  ParticleEmitterBuilder& maxParticles(size_t count) {
    if (emitter) {
      emitter->maxParticles = count;
      emitter->shaderParams.maxParticles = static_cast<float>(count);
    }
    return *this;
  }

  ParticleEmitterBuilder& particleLifetime(float lifetime) {
    if (emitter) {
      emitter->particleLifetime = lifetime;
      emitter->shaderParams.particleLifetime = lifetime;
    }
    return *this;
  }

  ParticleEmitterBuilder& material(MaterialID id) {
    if (emitter) emitter->materialID = id;
    return *this;
  }

  ParticleEmitterBuilder& active(bool isActive) {
    if (emitter) emitter->active = isActive;
    return *this;
  }

  ParticleEmitterBuilder& windInfluence(float influence) {
    if (emitter) {
      emitter->windInfluence = influence;
      emitter->shaderParams.windInfluence = influence;
    }
    return *this;
  }

 protected:
  ParticleEmitter* emitter;
};