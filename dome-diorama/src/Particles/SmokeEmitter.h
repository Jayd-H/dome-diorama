#pragma once
#include "ParticleEmitter.h"

class SmokeEmitter final : public ParticleEmitter {
 public:
  SmokeEmitter()
      : baseColor(0.3f, 0.3f, 0.3f),
        tipColor(0.6f, 0.6f, 0.6f),
        upwardSpeed(1.5f),
        horizontalSpread(0.8f),
        spawnRadius(0.3f),
        particleScale(1.5f) {
    SmokeEmitter::updateShaderParams();
  }

  void setBaseColor(const glm::vec3& color) { baseColor = color; }
  void setTipColor(const glm::vec3& color) { tipColor = color; }
  void setUpwardSpeed(float speed) { upwardSpeed = speed; }
  void setHorizontalSpread(float spread) { horizontalSpread = spread; }
  void setSpawnRadius(float radius) { spawnRadius = radius; }
  void setParticleScale(float scale) { particleScale = scale; }

 protected:
  void updateShaderParams() final {
    shaderParams.baseColor = baseColor;
    shaderParams.tipColor = tipColor;
    shaderParams.waveFrequency = 0.5f;
    shaderParams.waveAmplitude = horizontalSpread;
    shaderParams.upwardSpeed = upwardSpeed;
    shaderParams.particleScale = particleScale;
    shaderParams.spawnRadius = spawnRadius;
  }

 private:
  glm::vec3 baseColor;
  glm::vec3 tipColor;
  float upwardSpeed;
  float horizontalSpread;
  float spawnRadius;
  float particleScale;
};

class SmokeEmitterBuilder final : public ParticleEmitterBuilder {
 public:
  SmokeEmitterBuilder() : smokeEmitter(new SmokeEmitter()) {
    emitter = smokeEmitter;
  }

  ~SmokeEmitterBuilder() final = default;

  SmokeEmitterBuilder(const SmokeEmitterBuilder&) = delete;
  SmokeEmitterBuilder& operator=(const SmokeEmitterBuilder&) = delete;

  SmokeEmitterBuilder& name(const std::string& n) {
    ParticleEmitterBuilder::name(n);
    return *this;
  }

  SmokeEmitterBuilder& position(const glm::vec3& pos) {
    ParticleEmitterBuilder::position(pos);
    return *this;
  }

  SmokeEmitterBuilder& position(float x, float y, float z) {
    ParticleEmitterBuilder::position(x, y, z);
    return *this;
  }

  SmokeEmitterBuilder& maxParticles(size_t count) {
    ParticleEmitterBuilder::maxParticles(count);
    return *this;
  }

  SmokeEmitterBuilder& particleLifetime(float lifetime) {
    ParticleEmitterBuilder::particleLifetime(lifetime);
    return *this;
  }

  SmokeEmitterBuilder& material(MaterialID id) {
    ParticleEmitterBuilder::material(id);
    return *this;
  }

  SmokeEmitterBuilder& active(bool isActive) {
    ParticleEmitterBuilder::active(isActive);
    return *this;
  }

  SmokeEmitterBuilder& baseColor(const glm::vec3& color) {
    smokeEmitter->setBaseColor(color);
    return *this;
  }

  SmokeEmitterBuilder& baseColor(float r, float g, float b) {
    smokeEmitter->setBaseColor(glm::vec3(r, g, b));
    return *this;
  }

  SmokeEmitterBuilder& tipColor(const glm::vec3& color) {
    smokeEmitter->setTipColor(color);
    return *this;
  }

  SmokeEmitterBuilder& tipColor(float r, float g, float b) {
    smokeEmitter->setTipColor(glm::vec3(r, g, b));
    return *this;
  }

  SmokeEmitterBuilder& upwardSpeed(float speed) {
    smokeEmitter->setUpwardSpeed(speed);
    return *this;
  }

  SmokeEmitterBuilder& horizontalSpread(float spread) {
    smokeEmitter->setHorizontalSpread(spread);
    return *this;
  }

  SmokeEmitterBuilder& spawnRadius(float radius) {
    smokeEmitter->setSpawnRadius(radius);
    return *this;
  }

  SmokeEmitterBuilder& particleScale(float scale) {
    smokeEmitter->setParticleScale(scale);
    return *this;
  }

  SmokeEmitter* build() {
    SmokeEmitter* const result = smokeEmitter;
    smokeEmitter = nullptr;
    emitter = nullptr;
    return result;
  }

 private:
  SmokeEmitter* smokeEmitter;
};