#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

#include "Material.h"

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
  alignas(4) float waveFrequency;
  alignas(16) glm::vec3 tipColor;
  alignas(4) float waveAmplitude;
  alignas(16) glm::vec3 velocityBase;
  alignas(4) float particleLifetime;
  alignas(4) float upwardSpeed;
  alignas(4) float particleScale;
  alignas(4) float spawnRadius;
  alignas(4) float maxParticles;
};

class ParticleEmitter {
 public:
  std::string name;
  glm::vec3 position;
  bool active;

  ParticleEmitter();
  virtual ~ParticleEmitter() = default;

  virtual void update(float deltaTime);
  void setPosition(const glm::vec3& pos);
  void setActive(bool isActive);

  size_t getMaxParticles() const { return maxParticles; }
  MaterialID getMaterialID() const { return materialID; }

  const ParticleShaderParams& getShaderParams() const { return shaderParams; }

 protected:
  size_t maxParticles;
  float particleLifetime;
  MaterialID materialID;
  float time;

  ParticleShaderParams shaderParams;

  virtual void updateShaderParams() = 0;

  friend class ParticleEmitterBuilder;
};

class ParticleEmitterBuilder {
 public:
  ParticleEmitterBuilder();
  virtual ~ParticleEmitterBuilder() = default;

  ParticleEmitterBuilder& name(const std::string& name);
  ParticleEmitterBuilder& position(const glm::vec3& pos);
  ParticleEmitterBuilder& position(float x, float y, float z);
  ParticleEmitterBuilder& maxParticles(size_t count);
  ParticleEmitterBuilder& particleLifetime(float lifetime);
  ParticleEmitterBuilder& material(MaterialID id);
  ParticleEmitterBuilder& active(bool isActive);

 protected:
  ParticleEmitter* emitter;
};