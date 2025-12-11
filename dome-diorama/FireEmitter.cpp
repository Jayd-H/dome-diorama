#include "FireEmitter.h"

FireEmitter::FireEmitter()
    : waveFrequency(2.0f),
      waveAmplitude(0.5f),
      upwardSpeed(2.0f),
      spawnRadius(0.2f),
      particleScale(0.5f),
      baseColor(1.0f, 0.9f, 0.1f),
      tipColor(1.0f, 0.3f, 0.0f) {
  updateShaderParams();
}

void FireEmitter::setWaveFrequency(float frequency) {
  waveFrequency = frequency;
}

void FireEmitter::setWaveAmplitude(float amplitude) {
  waveAmplitude = amplitude;
}

void FireEmitter::setBaseColor(const glm::vec3& color) { baseColor = color; }

void FireEmitter::setTipColor(const glm::vec3& color) { tipColor = color; }

void FireEmitter::setUpwardSpeed(float speed) { upwardSpeed = speed; }

void FireEmitter::setSpawnRadius(float radius) { spawnRadius = radius; }

void FireEmitter::setParticleScale(float scale) { particleScale = scale; }

void FireEmitter::updateShaderParams() {
  shaderParams.baseColor = baseColor;
  shaderParams.tipColor = tipColor;
  shaderParams.waveFrequency = waveFrequency;
  shaderParams.waveAmplitude = waveAmplitude;
  shaderParams.upwardSpeed = upwardSpeed;
  shaderParams.particleScale = particleScale;
  shaderParams.spawnRadius = spawnRadius;
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

FireEmitterBuilder& FireEmitterBuilder::spawnRadius(float radius) {
  fireEmitter->setSpawnRadius(radius);
  return *this;
}

FireEmitterBuilder& FireEmitterBuilder::particleScale(float scale) {
  fireEmitter->setParticleScale(scale);
  return *this;
}

FireEmitter* FireEmitterBuilder::build() { return fireEmitter; }