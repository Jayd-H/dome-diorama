#include "ParticleEmitter.h"

#include <glm/gtc/matrix_transform.hpp>

ParticleEmitter::ParticleEmitter()
    : name("Unnamed Emitter"),
      position(0.0f),
      active(true),
      maxParticles(1000),
      activeParticleCount(0),
      spawnRate(50.0f),
      spawnAccumulator(0.0f),
      particleLifetime(2.0f),
      materialID(INVALID_MATERIAL_ID) {
  particles.resize(maxParticles);
  instanceData.reserve(maxParticles);

  for (auto& particle : particles) {
    particle.active = false;
  }
}

void ParticleEmitter::update(float deltaTime) {
  if (!active) return;

  spawnAccumulator += deltaTime;
  float spawnInterval = 1.0f / spawnRate;

  while (spawnAccumulator >= spawnInterval) {
    spawnParticle();
    spawnAccumulator -= spawnInterval;
  }

  activeParticleCount = 0;
  for (auto& particle : particles) {
    if (!particle.active) continue;

    particle.age += deltaTime;

    if (particle.age >= particle.lifetime) {
      particle.active = false;
      continue;
    }

    updateParticle(particle, deltaTime);
    activeParticleCount++;
  }

  updateInstanceData();
}

void ParticleEmitter::setPosition(const glm::vec3& pos) { position = pos; }

void ParticleEmitter::setActive(bool isActive) { active = isActive; }

void ParticleEmitter::spawnParticle() {
  for (auto& particle : particles) {
    if (!particle.active) {
      particle.active = true;
      particle.age = 0.0f;
      particle.lifetime = particleLifetime;
      initializeParticle(particle);
      return;
    }
  }
}

void ParticleEmitter::updateInstanceData() {
  instanceData.clear();

  for (const auto& particle : particles) {
    if (!particle.active) continue;

    ParticleInstanceData instance;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, particle.position);
    model = glm::scale(model, glm::vec3(particle.scale));

    instance.modelMatrix = model;
    instance.color = particle.color;

    instanceData.push_back(instance);
  }
}

ParticleEmitterBuilder::ParticleEmitterBuilder() : emitter(nullptr) {}

ParticleEmitterBuilder& ParticleEmitterBuilder::name(const std::string& name) {
  if (emitter) emitter->name = name;
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::position(const glm::vec3& pos) {
  if (emitter) emitter->position = pos;
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::position(float x, float y,
                                                         float z) {
  if (emitter) emitter->position = glm::vec3(x, y, z);
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::maxParticles(size_t count) {
  if (emitter) {
    emitter->maxParticles = count;
    emitter->particles.resize(count);
    emitter->instanceData.reserve(count);
    for (auto& particle : emitter->particles) {
      particle.active = false;
    }
  }
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::spawnRate(float rate) {
  if (emitter) emitter->spawnRate = rate;
  return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::particleLifetime(
    float lifetime) {
  if (emitter) emitter->particleLifetime = lifetime;
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