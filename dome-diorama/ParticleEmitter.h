#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

#include "Material.h"

using EmitterID = uint32_t;
constexpr EmitterID INVALID_EMITTER_ID = 0;

struct ParticleData {
  glm::vec3 position;
  glm::vec3 velocity;
  glm::vec4 color;
  float lifetime;
  float age;
  float scale;
  bool active;
};

struct ParticleInstanceData {
  glm::mat4 modelMatrix;
  glm::vec4 color;
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

  const std::vector<ParticleInstanceData>& getInstanceData() const {
    return instanceData;
  }
  size_t getActiveParticleCount() const { return activeParticleCount; }

  MaterialID getMaterialID() const { return materialID; }

 protected:
  std::vector<ParticleData> particles;
  std::vector<ParticleInstanceData> instanceData;
  size_t maxParticles;
  size_t activeParticleCount;
  float spawnRate;
  float spawnAccumulator;
  float particleLifetime;
  MaterialID materialID;

  virtual void initializeParticle(ParticleData& particle) = 0;
  virtual void updateParticle(ParticleData& particle, float deltaTime) = 0;

  void spawnParticle();
  void updateInstanceData();

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
  ParticleEmitterBuilder& spawnRate(float rate);
  ParticleEmitterBuilder& particleLifetime(float lifetime);
  ParticleEmitterBuilder& material(MaterialID id);
  ParticleEmitterBuilder& active(bool isActive);

 protected:
  ParticleEmitter* emitter;
};