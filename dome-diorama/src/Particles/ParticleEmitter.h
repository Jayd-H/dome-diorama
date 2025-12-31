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
  alignas(16) glm::vec3 baseColor;
  alignas(16) glm::vec3 tipColor;
  alignas(16) glm::vec3 velocityBase;
  alignas(4) float time;
  alignas(4) float waveFrequency;
  alignas(4) float waveAmplitude;
  alignas(4) float particleLifetime;
  alignas(4) float upwardSpeed;
  alignas(4) float particleScale;
  alignas(4) float spawnRadius;
  alignas(4) float maxParticles;
};

class ParticleEmitter {
 public:
  ParticleEmitter()
      : maxParticles(1000),
        particleLifetime(2.0f),
        materialID(INVALID_MATERIAL_ID),
        time(0.0f),
        shaderParams{},
        name("Unnamed Emitter"),
        position(0.0f),
        active(true) {
    shaderParams.emitterPosition = position;
    shaderParams.time = 0.0f;
    shaderParams.particleLifetime = particleLifetime;
    shaderParams.maxParticles = static_cast<float>(maxParticles);
  }

  virtual ~ParticleEmitter() = default;

  virtual void update(float deltaTime) {
    if (!active) return;
    time += deltaTime;
    shaderParams.time = time;
    shaderParams.emitterPosition = position;
    updateShaderParams();
  }

  void setPosition(const glm::vec3& pos) {
    position = pos;
    shaderParams.emitterPosition = pos;
  }

  void setActive(bool isActive) { active = isActive; }

  const std::string& getName() const { return name; }
  void setName(const std::string& n) { name = n; }

  const glm::vec3& getPosition() const { return position; }
  bool isActive() const { return active; }

  size_t getMaxParticles() const { return maxParticles; }
  MaterialID getMaterialID() const { return materialID; }
  ParticleShaderParams getShaderParams() const { return shaderParams; }

 protected:
  size_t maxParticles;
  float particleLifetime;
  MaterialID materialID;
  float time;
  ParticleShaderParams shaderParams;

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

 protected:
  ParticleEmitter* emitter;
};