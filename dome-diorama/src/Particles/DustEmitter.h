#pragma once
#include "ParticleEmitter.h"

class DustEmitter final : public ParticleEmitter {
 public:
  DustEmitter()
      : baseColor(0.7f, 0.6f, 0.5f),
        tipColor(0.5f, 0.4f, 0.3f),
        swirlingSpeed(1.0f),
        driftSpeed(0.5f),
        spawnRadius(5.0f),
        particleScale(0.3f),
        fadeInDuration(0.3f),
        fadeOutDuration(0.5f),
        scaleOverLifetime(1.2f),
        rotationSpeed(0.3f) {
    DustEmitter::updateShaderParams();
  }

  void setBaseColor(const glm::vec3& color) { baseColor = color; }
  void setTipColor(const glm::vec3& color) { tipColor = color; }
  void setSwirlingSpeed(float speed) { swirlingSpeed = speed; }
  void setDriftSpeed(float speed) { driftSpeed = speed; }
  void setSpawnRadius(float radius) { spawnRadius = radius; }
  void setParticleScale(float scale) { particleScale = scale; }
  void setFadeInDuration(float duration) { fadeInDuration = duration; }
  void setFadeOutDuration(float duration) { fadeOutDuration = duration; }
  void setScaleOverLifetime(float scale) { scaleOverLifetime = scale; }
  void setRotationSpeed(float speed) { rotationSpeed = speed; }

 protected:
  void updateShaderParams() final {
    shaderParams.baseColor = baseColor;
    shaderParams.tipColor = tipColor;
    shaderParams.gravity = glm::vec3(0.0f);
    shaderParams.initialVelocity = glm::vec3(swirlingSpeed, driftSpeed, 0.0f);
    shaderParams.spawnRadius = spawnRadius;
    shaderParams.particleScale = particleScale;
    shaderParams.fadeInDuration = fadeInDuration;
    shaderParams.fadeOutDuration = fadeOutDuration;
    shaderParams.billboardMode = static_cast<int>(BillboardMode::Spherical);
    shaderParams.colorMode = static_cast<int>(ColorMode::Gradient);
    shaderParams.velocityRandomness = 1.0f;
    shaderParams.scaleOverLifetime = scaleOverLifetime;
    shaderParams.rotationSpeed = rotationSpeed;
  }

 private:
  glm::vec3 baseColor;
  glm::vec3 tipColor;
  float swirlingSpeed;
  float driftSpeed;
  float spawnRadius;
  float particleScale;
  float fadeInDuration;
  float fadeOutDuration;
  float scaleOverLifetime;
  float rotationSpeed;
};

class DustEmitterBuilder final : public ParticleEmitterBuilder {
 public:
  DustEmitterBuilder() : dustEmitter(new DustEmitter()) {
    emitter = dustEmitter;
  }

  ~DustEmitterBuilder() final = default;

  DustEmitterBuilder(const DustEmitterBuilder&) = delete;
  DustEmitterBuilder& operator=(const DustEmitterBuilder&) = delete;

  DustEmitterBuilder& name(const std::string& n) {
    ParticleEmitterBuilder::name(n);
    return *this;
  }

  DustEmitterBuilder& position(const glm::vec3& pos) {
    ParticleEmitterBuilder::position(pos);
    return *this;
  }

  DustEmitterBuilder& position(float x, float y, float z) {
    ParticleEmitterBuilder::position(x, y, z);
    return *this;
  }

  DustEmitterBuilder& maxParticles(size_t count) {
    ParticleEmitterBuilder::maxParticles(count);
    return *this;
  }

  DustEmitterBuilder& particleLifetime(float lifetime) {
    ParticleEmitterBuilder::particleLifetime(lifetime);
    return *this;
  }

  DustEmitterBuilder& material(MaterialID id) {
    ParticleEmitterBuilder::material(id);
    return *this;
  }

  DustEmitterBuilder& active(bool isActive) {
    ParticleEmitterBuilder::active(isActive);
    return *this;
  }

  DustEmitterBuilder& baseColor(const glm::vec3& color) {
    dustEmitter->setBaseColor(color);
    return *this;
  }

  DustEmitterBuilder& baseColor(float r, float g, float b) {
    dustEmitter->setBaseColor(glm::vec3(r, g, b));
    return *this;
  }

  DustEmitterBuilder& tipColor(const glm::vec3& color) {
    dustEmitter->setTipColor(color);
    return *this;
  }

  DustEmitterBuilder& tipColor(float r, float g, float b) {
    dustEmitter->setTipColor(glm::vec3(r, g, b));
    return *this;
  }

  DustEmitterBuilder& swirlingSpeed(float speed) {
    dustEmitter->setSwirlingSpeed(speed);
    return *this;
  }

  DustEmitterBuilder& driftSpeed(float speed) {
    dustEmitter->setDriftSpeed(speed);
    return *this;
  }

  DustEmitterBuilder& spawnRadius(float radius) {
    dustEmitter->setSpawnRadius(radius);
    return *this;
  }

  DustEmitterBuilder& particleScale(float scale) {
    dustEmitter->setParticleScale(scale);
    return *this;
  }

  DustEmitterBuilder& fadeInDuration(float duration) {
    dustEmitter->setFadeInDuration(duration);
    return *this;
  }

  DustEmitterBuilder& fadeOutDuration(float duration) {
    dustEmitter->setFadeOutDuration(duration);
    return *this;
  }

  DustEmitterBuilder& scaleOverLifetime(float scale) {
    dustEmitter->setScaleOverLifetime(scale);
    return *this;
  }

  DustEmitterBuilder& rotationSpeed(float speed) {
    dustEmitter->setRotationSpeed(speed);
    return *this;
  }

  DustEmitter* build() {
    DustEmitter* const result = dustEmitter;
    dustEmitter = nullptr;
    emitter = nullptr;
    return result;
  }

 private:
  DustEmitter* dustEmitter;
};