#pragma once
#include "ParticleEmitter.h"

class FireEmitter final : public ParticleEmitter {
 public:
  FireEmitter()
      : baseColor(1.0f, 0.9f, 0.1f),
        tipColor(1.0f, 0.3f, 0.0f),
        waveFrequency(2.0f),
        waveAmplitude(0.5f),
        upwardSpeed(2.0f),
        spawnRadius(0.2f),
        particleScale(0.5f) {
    FireEmitter::updateShaderParams();
  }

  void setWaveFrequency(float frequency) { waveFrequency = frequency; }
  void setWaveAmplitude(float amplitude) { waveAmplitude = amplitude; }
  void setBaseColor(const glm::vec3& color) { baseColor = color; }
  void setTipColor(const glm::vec3& color) { tipColor = color; }
  void setUpwardSpeed(float speed) { upwardSpeed = speed; }
  void setSpawnRadius(float radius) { spawnRadius = radius; }
  void setParticleScale(float scale) { particleScale = scale; }

 protected:
  void updateShaderParams() final {
    shaderParams.baseColor = baseColor;
    shaderParams.tipColor = tipColor;
    shaderParams.waveFrequency = waveFrequency;
    shaderParams.waveAmplitude = waveAmplitude;
    shaderParams.upwardSpeed = upwardSpeed;
    shaderParams.particleScale = particleScale;
    shaderParams.spawnRadius = spawnRadius;
  }

 private:
  glm::vec3 baseColor;
  glm::vec3 tipColor;
  float waveFrequency;
  float waveAmplitude;
  float upwardSpeed;
  float spawnRadius;
  float particleScale;
};

class FireEmitterBuilder final : public ParticleEmitterBuilder {
 public:
  FireEmitterBuilder() : fireEmitter(new FireEmitter()) {
    emitter = fireEmitter;
  }

  ~FireEmitterBuilder() final = default;

  FireEmitterBuilder(const FireEmitterBuilder&) = delete;
  FireEmitterBuilder& operator=(const FireEmitterBuilder&) = delete;

  FireEmitterBuilder& name(const std::string& n) {
    ParticleEmitterBuilder::name(n);
    return *this;
  }

  FireEmitterBuilder& position(const glm::vec3& pos) {
    ParticleEmitterBuilder::position(pos);
    return *this;
  }

  FireEmitterBuilder& position(float x, float y, float z) {
    ParticleEmitterBuilder::position(x, y, z);
    return *this;
  }

  FireEmitterBuilder& maxParticles(size_t count) {
    ParticleEmitterBuilder::maxParticles(count);
    return *this;
  }

  FireEmitterBuilder& particleLifetime(float lifetime) {
    ParticleEmitterBuilder::particleLifetime(lifetime);
    return *this;
  }

  FireEmitterBuilder& material(MaterialID id) {
    ParticleEmitterBuilder::material(id);
    return *this;
  }

  FireEmitterBuilder& active(bool isActive) {
    ParticleEmitterBuilder::active(isActive);
    return *this;
  }

  FireEmitterBuilder& waveFrequency(float frequency) {
    fireEmitter->setWaveFrequency(frequency);
    return *this;
  }

  FireEmitterBuilder& waveAmplitude(float amplitude) {
    fireEmitter->setWaveAmplitude(amplitude);
    return *this;
  }

  FireEmitterBuilder& baseColor(const glm::vec3& color) {
    fireEmitter->setBaseColor(color);
    return *this;
  }

  FireEmitterBuilder& baseColor(float r, float g, float b) {
    fireEmitter->setBaseColor(glm::vec3(r, g, b));
    return *this;
  }

  FireEmitterBuilder& tipColor(const glm::vec3& color) {
    fireEmitter->setTipColor(color);
    return *this;
  }

  FireEmitterBuilder& tipColor(float r, float g, float b) {
    fireEmitter->setTipColor(glm::vec3(r, g, b));
    return *this;
  }

  FireEmitterBuilder& upwardSpeed(float speed) {
    fireEmitter->setUpwardSpeed(speed);
    return *this;
  }

  FireEmitterBuilder& spawnRadius(float radius) {
    fireEmitter->setSpawnRadius(radius);
    return *this;
  }

  FireEmitterBuilder& particleScale(float scale) {
    fireEmitter->setParticleScale(scale);
    return *this;
  }

  FireEmitter* build() {
    FireEmitter* const result = fireEmitter;
    fireEmitter = nullptr;
    emitter = nullptr;
    return result;
  }

 private:
  FireEmitter* fireEmitter;
};