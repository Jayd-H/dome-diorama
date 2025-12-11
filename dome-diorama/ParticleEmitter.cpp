#include "ParticleEmitter.h"

#include <glm/gtc/matrix_transform.hpp>

ParticleEmitter::ParticleEmitter()
    : name("Unnamed Emitter"),
      position(0.0f),
      active(true),
      maxParticles(1000),
      particleLifetime(2.0f),
      materialID(INVALID_MATERIAL_ID),
      time(0.0f) {
  shaderParams = {};
  shaderParams.emitterPosition = position;
  shaderParams.time = 0.0f;
  shaderParams.particleLifetime = particleLifetime;
  shaderParams.maxParticles = static_cast<float>(maxParticles);
}

void ParticleEmitter::update(float deltaTime) {
  if (!active) return;

  time += deltaTime;
  shaderParams.time = time;
  shaderParams.emitterPosition = position;

  updateShaderParams();
}

void ParticleEmitter::setPosition(const glm::vec3& pos) {
  position = pos;
  shaderParams.emitterPosition = pos;
}

void ParticleEmitter::setActive(bool isActive) { active = isActive; }

ParticleEmitterBuilder::ParticleEmitterBuilder() : emitter(nullptr) {}

ParticleEmitterBuilder& ParticleEmitterBuilder::name(const std::string& name) {
  if (emitter) emitter->name = name;
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::position(const glm::vec3& pos) {
  if (emitter) {
    emitter->position = pos;
    emitter->shaderParams.emitterPosition = pos;
  }
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::position(float x, float y,
                                                         float z) {
  if (emitter) {
    emitter->position = glm::vec3(x, y, z);
    emitter->shaderParams.emitterPosition = glm::vec3(x, y, z);
  }
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::maxParticles(size_t count) {
  if (emitter) {
    emitter->maxParticles = count;
    emitter->shaderParams.maxParticles = static_cast<float>(count);
  }
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::particleLifetime(
    float lifetime) {
  if (emitter) {
    emitter->particleLifetime = lifetime;
    emitter->shaderParams.particleLifetime = lifetime;
  }
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::material(MaterialID id) {
  if (emitter) emitter->materialID = id;
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::active(bool isActive) {
  if (emitter) emitter->active = isActive;
  return *this;
}