#pragma once
#include "ParticleEmitter.h"

class RainEmitter final : public ParticleEmitter {
 public:
  RainEmitter()
      : baseColor(0.6f, 0.7f, 0.9f),
        tipColor(0.8f, 0.9f, 1.0f),
        downwardSpeed(15.0f),
        windStrength(2.0f),
        spawnRadius(20.0f),
        particleScale(0.1f),
        fadeInDuration(0.0f),
        fadeOutDuration(0.1f) {
    RainEmitter::updateShaderParams();
  }

  void setBaseColor(const glm::vec3& color) { baseColor = color; }
  void setTipColor(const glm::vec3& color) { tipColor = color; }
  void setDownwardSpeed(float speed) { downwardSpeed = speed; }
  void setWindStrength(float strength) { windStrength = strength; }
  void setSpawnRadius(float radius) { spawnRadius = radius; }
  void setParticleScale(float scale) { particleScale = scale; }
  void setFadeInDuration(float duration) { fadeInDuration = duration; }
  void setFadeOutDuration(float duration) { fadeOutDuration = duration; }

 protected:
  void updateShaderParams() final {
    shaderParams.baseColor = baseColor;
    shaderParams.tipColor = tipColor;
    shaderParams.gravity = glm::vec3(0.0f, -downwardSpeed, 0.0f);
    shaderParams.initialVelocity = glm::vec3(windStrength, 0.0f, 0.0f);
    shaderParams.spawnRadius = spawnRadius;
    shaderParams.particleScale = particleScale;
    shaderParams.fadeInDuration = fadeInDuration;
    shaderParams.fadeOutDuration = fadeOutDuration;
    shaderParams.billboardMode = static_cast<int>(BillboardMode::Cylindrical);
    shaderParams.colorMode = static_cast<int>(ColorMode::BaseOnly);
    shaderParams.velocityRandomness = 0.2f;
    shaderParams.scaleOverLifetime = 1.0f;
    shaderParams.rotationSpeed = 0.0f;
  }

 private:
  glm::vec3 baseColor;
  glm::vec3 tipColor;
  float downwardSpeed;
  float windStrength;
  float spawnRadius;
  float particleScale;
  float fadeInDuration;
  float fadeOutDuration;
};

class RainEmitterBuilder final : public ParticleEmitterBuilder {
 public:
  RainEmitterBuilder() : rainEmitter(new RainEmitter()) {
    emitter = rainEmitter;
  }

  ~RainEmitterBuilder() final = default;

  RainEmitterBuilder(const RainEmitterBuilder&) = delete;
  RainEmitterBuilder& operator=(const RainEmitterBuilder&) = delete;

  RainEmitterBuilder& name(const std::string& n) {
    ParticleEmitterBuilder::name(n);
    return *this;
  }

  RainEmitterBuilder& position(const glm::vec3& pos) {
    ParticleEmitterBuilder::position(pos);
    return *this;
  }

  RainEmitterBuilder& position(float x, float y, float z) {
    ParticleEmitterBuilder::position(x, y, z);
    return *this;
  }

  RainEmitterBuilder& maxParticles(size_t count) {
    ParticleEmitterBuilder::maxParticles(count);
    return *this;
  }

  RainEmitterBuilder& particleLifetime(float lifetime) {
    ParticleEmitterBuilder::particleLifetime(lifetime);
    return *this;
  }

  RainEmitterBuilder& material(MaterialID id) {
    ParticleEmitterBuilder::material(id);
    return *this;
  }

  RainEmitterBuilder& active(bool isActive) {
    ParticleEmitterBuilder::active(isActive);
    return *this;
  }

  RainEmitterBuilder& baseColor(const glm::vec3& color) {
    rainEmitter->setBaseColor(color);
    return *this;
  }

  RainEmitterBuilder& baseColor(float r, float g, float b) {
    rainEmitter->setBaseColor(glm::vec3(r, g, b));
    return *this;
  }

  RainEmitterBuilder& tipColor(const glm::vec3& color) {
    rainEmitter->setTipColor(color);
    return *this;
  }

  RainEmitterBuilder& tipColor(float r, float g, float b) {
    rainEmitter->setTipColor(glm::vec3(r, g, b));
    return *this;
  }

  RainEmitterBuilder& downwardSpeed(float speed) {
    rainEmitter->setDownwardSpeed(speed);
    return *this;
  }

  RainEmitterBuilder& windStrength(float strength) {
    rainEmitter->setWindStrength(strength);
    return *this;
  }

  RainEmitterBuilder& spawnRadius(float radius) {
    rainEmitter->setSpawnRadius(radius);
    return *this;
  }

  RainEmitterBuilder& particleScale(float scale) {
    rainEmitter->setParticleScale(scale);
    return *this;
  }

  RainEmitterBuilder& fadeInDuration(float duration) {
    rainEmitter->setFadeInDuration(duration);
    return *this;
  }

  RainEmitterBuilder& fadeOutDuration(float duration) {
    rainEmitter->setFadeOutDuration(duration);
    return *this;
  }

  RainEmitterBuilder& windInfluence(float influence) {
    rainEmitter->setWindInfluence(influence);
    return *this;
  }

  RainEmitter* build() {
    RainEmitter* const result = rainEmitter;
    rainEmitter = nullptr;
    emitter = nullptr;
    return result;
  }

 private:
  RainEmitter* rainEmitter;
};