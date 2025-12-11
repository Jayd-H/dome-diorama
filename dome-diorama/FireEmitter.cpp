#include "FireEmitter.h"

#include <cmath>
#include <random>

static std::random_device rd;
static std::mt19937 gen(rd());

static float randomFloat(float min, float max) {
  std::uniform_real_distribution<float> dis(min, max);
  return dis(gen);
}

FireEmitter::FireEmitter()
    : waveFrequency(2.0f),
      waveAmplitude(0.5f),
      upwardSpeed(2.0f),
      baseColor(1.0f, 0.9f, 0.1f),
      tipColor(1.0f, 0.3f, 0.0f) {}

void FireEmitter::initializeParticle(ParticleData& particle) {
  particle.position = position;
  particle.position.x += randomFloat(-0.2f, 0.2f);
  particle.position.z += randomFloat(-0.2f, 0.2f);

  particle.velocity = glm::vec3(0.0f, upwardSpeed, 0.0f);
  particle.velocity.x = randomFloat(-0.5f, 0.5f);
  particle.velocity.z = randomFloat(-0.5f, 0.5f);

  particle.color = glm::vec4(baseColor, 1.0f);
  particle.scale = randomFloat(0.3f, 0.6f);
}

void FireEmitter::updateParticle(ParticleData& particle, float deltaTime) {
  float lifeRatio = particle.age / particle.lifetime;

  float sineWave =
      sin(particle.age * waveFrequency + particle.position.x * 2.0f);
  particle.velocity.x += sineWave * waveAmplitude * deltaTime;

  particle.position += particle.velocity * deltaTime;

  glm::vec3 currentColor = glm::mix(baseColor, tipColor, lifeRatio);
  float alpha = 1.0f - lifeRatio;

  particle.color = glm::vec4(currentColor, alpha);

  particle.scale *= (1.0f - deltaTime * 0.5f);
}

FireEmitterBuilder::FireEmitterBuilder() {
  fireEmitter = new FireEmitter();
  emitter = fireEmitter;
}

FireEmitterBuilder& FireEmitterBuilder::name(const std::string& name) {
  ParticleEmitterBuilder::name(name);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::position(const glm::vec3& pos) {
  ParticleEmitterBuilder::position(pos);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::position(float x, float y, float z) {
  ParticleEmitterBuilder::position(x, y, z);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::maxParticles(size_t count) {
  ParticleEmitterBuilder::maxParticles(count);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::spawnRate(float rate) {
  ParticleEmitterBuilder::spawnRate(rate);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::particleLifetime(float lifetime) {
  ParticleEmitterBuilder::particleLifetime(lifetime);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::material(MaterialID id) {
  ParticleEmitterBuilder::material(id);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::active(bool isActive) {
  ParticleEmitterBuilder::active(isActive);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::waveFrequency(float frequency) {
  fireEmitter->setWaveFrequency(frequency);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::waveAmplitude(float amplitude) {
  fireEmitter->setWaveAmplitude(amplitude);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::baseColor(const glm::vec3& color) {
  fireEmitter->setBaseColor(color);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::baseColor(float r, float g, float b) {
  fireEmitter->setBaseColor(glm::vec3(r, g, b));
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::tipColor(const glm::vec3& color) {
  fireEmitter->setTipColor(color);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::tipColor(float r, float g, float b) {
  fireEmitter->setTipColor(glm::vec3(r, g, b));
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::upwardSpeed(float speed) {
  fireEmitter->setUpwardSpeed(speed);
  return *this;
}

FireEmitter* FireEmitterBuilder::build() { return fireEmitter; }