#pragma once
#include <memory>
#include <string>
#include <utility>

#include "ParticleEmitter.h"

struct ParticleEmitterConfig {
  glm::vec3 baseColor;
  glm::vec3 tipColor;
  glm::vec3 gravity;
  glm::vec3 initialVelocity;
  const char* name;
  float spawnRadius;
  float particleScale;
  float fadeIn;
  float fadeOut;
  float velocityRandomness;
  float scaleOverLifetime;
  float rotationSpeed;
  ParticleEmitter::BillboardMode billboardMode;
  ParticleEmitter::ColorMode colorMode;

  ParticleEmitterConfig() = default;
  ParticleEmitterConfig(const ParticleEmitterConfig&) = default;
  ParticleEmitterConfig& operator=(const ParticleEmitterConfig&) = default;
  ParticleEmitterConfig(ParticleEmitterConfig&&) = default;
  ParticleEmitterConfig& operator=(ParticleEmitterConfig&&) = default;
  ~ParticleEmitterConfig() = default;
};

class ParticleEmitterBuilder final {
 public:
  ParticleEmitterBuilder() : emitter(std::make_unique<ParticleEmitter>()) {}
  ~ParticleEmitterBuilder() = default;

  ParticleEmitterBuilder(ParticleEmitterBuilder&& other) noexcept = default;
  ParticleEmitterBuilder& operator=(ParticleEmitterBuilder&& other) noexcept =
      default;

  ParticleEmitterBuilder(const ParticleEmitterBuilder&) = delete;
  ParticleEmitterBuilder& operator=(const ParticleEmitterBuilder&) = delete;

  ParticleEmitterBuilder& name(const std::string& n) {
    if (emitter) emitter->setName(n);
    return *this;
  }

  ParticleEmitterBuilder& position(const glm::vec3& pos) {
    if (emitter) emitter->setPosition(pos);
    return *this;
  }

  ParticleEmitterBuilder& position(float x, float y, float z) {
    if (emitter) emitter->setPosition(glm::vec3(x, y, z));
    return *this;
  }

  ParticleEmitterBuilder& maxParticles(size_t count) {
    if (emitter) emitter->setMaxParticles(count);
    return *this;
  }

  ParticleEmitterBuilder& particleLifetime(float lifetime) {
    if (emitter) emitter->setParticleLifetime(lifetime);
    return *this;
  }

  ParticleEmitterBuilder& material(MaterialID id) {
    if (emitter) emitter->setMaterialID(id);
    return *this;
  }

  ParticleEmitterBuilder& active(bool isActive) {
    if (emitter) emitter->setActive(isActive);
    return *this;
  }

  ParticleEmitterBuilder& baseColor(const glm::vec3& color) {
    if (emitter) emitter->setBaseColor(color);
    return *this;
  }

  ParticleEmitterBuilder& tipColor(const glm::vec3& color) {
    if (emitter) emitter->setTipColor(color);
    return *this;
  }

  ParticleEmitterBuilder& gravity(const glm::vec3& g) {
    if (emitter) emitter->setGravity(g);
    return *this;
  }

  ParticleEmitterBuilder& initialVelocity(const glm::vec3& v) {
    if (emitter) emitter->setInitialVelocity(v);
    return *this;
  }

  ParticleEmitterBuilder& velocityRandomness(float r) {
    if (emitter) emitter->setVelocityRandomness(r);
    return *this;
  }

  ParticleEmitterBuilder& spawnRadius(float radius) {
    if (emitter) emitter->setSpawnRadius(radius);
    return *this;
  }

  ParticleEmitterBuilder& particleScale(float scale) {
    if (emitter) emitter->setParticleScale(scale);
    return *this;
  }

  ParticleEmitterBuilder& scaleOverLifetime(float scale) {
    if (emitter) emitter->setScaleOverLifetime(scale);
    return *this;
  }

  ParticleEmitterBuilder& rotationSpeed(float speed) {
    if (emitter) emitter->setRotationSpeed(speed);
    return *this;
  }

  ParticleEmitterBuilder& fadeTimings(float in, float out) {
    if (emitter) {
      emitter->setFadeInDuration(in);
      emitter->setFadeOutDuration(out);
    }
    return *this;
  }

  ParticleEmitterBuilder& billboardMode(ParticleEmitter::BillboardMode mode) {
    if (emitter) emitter->setBillboardMode(mode);
    return *this;
  }

  ParticleEmitterBuilder& colorMode(ParticleEmitter::ColorMode mode) {
    if (emitter) emitter->setColorMode(mode);
    return *this;
  }

  ParticleEmitterBuilder& windInfluence(float influence) {
    if (emitter) emitter->setWindInfluence(influence);
    return *this;
  }

  ParticleEmitter* build() { return emitter.release(); }

 private:
  std::unique_ptr<ParticleEmitter> emitter;
};

class EmitterPresets final {
 public:
  static ParticleEmitterBuilder createFire();
  static ParticleEmitterBuilder createSmoke();
  static ParticleEmitterBuilder createDust();
  static ParticleEmitterBuilder createRain();
  static ParticleEmitterBuilder createSnow();

 private:
  static ParticleEmitterBuilder createFromConfig(
      const ParticleEmitterConfig& config);
};

inline ParticleEmitterBuilder EmitterPresets::createFromConfig(
    const ParticleEmitterConfig& config) {
  ParticleEmitterBuilder builder;
  builder.name(config.name)
      .baseColor(config.baseColor)
      .tipColor(config.tipColor)
      .gravity(config.gravity)
      .initialVelocity(config.initialVelocity)
      .spawnRadius(config.spawnRadius)
      .particleScale(config.particleScale)
      .fadeTimings(config.fadeIn, config.fadeOut)
      .velocityRandomness(config.velocityRandomness)
      .scaleOverLifetime(config.scaleOverLifetime)
      .rotationSpeed(config.rotationSpeed)
      .billboardMode(config.billboardMode)
      .colorMode(config.colorMode);
  return builder;
}

inline ParticleEmitterBuilder EmitterPresets::createFire() {
  ParticleEmitterConfig config;
  config.name = "Fire Emitter";
  config.baseColor = glm::vec3(1.0f, 0.9f, 0.1f);
  config.tipColor = glm::vec3(1.0f, 0.3f, 0.0f);
  config.gravity = glm::vec3(0.0f);
  config.initialVelocity = glm::vec3(0.0f, 2.0f, 0.0f);
  config.spawnRadius = 0.2f;
  config.particleScale = 0.5f;
  config.fadeIn = 0.1f;
  config.fadeOut = 0.5f;
  config.velocityRandomness = 0.5f;
  config.scaleOverLifetime = 0.3f;
  config.rotationSpeed = 1.0f;
  config.billboardMode = ParticleEmitter::BillboardMode::Spherical;
  config.colorMode = ParticleEmitter::ColorMode::Gradient;
  return createFromConfig(config);
}

inline ParticleEmitterBuilder EmitterPresets::createSmoke() {
  ParticleEmitterConfig config;
  config.name = "Smoke Emitter";
  config.baseColor = glm::vec3(0.3f, 0.3f, 0.3f);
  config.tipColor = glm::vec3(0.6f, 0.6f, 0.6f);
  config.gravity = glm::vec3(0.0f);
  config.initialVelocity = glm::vec3(0.0f, 1.5f, 0.0f);
  config.spawnRadius = 0.3f;
  config.particleScale = 1.5f;
  config.fadeIn = 0.2f;
  config.fadeOut = 0.8f;
  config.velocityRandomness = 0.8f;
  config.scaleOverLifetime = 2.5f;
  config.rotationSpeed = 0.5f;
  config.billboardMode = ParticleEmitter::BillboardMode::Spherical;
  config.colorMode = ParticleEmitter::ColorMode::Gradient;
  return createFromConfig(config);
}

inline ParticleEmitterBuilder EmitterPresets::createDust() {
  ParticleEmitterConfig config;
  config.name = "Dust Emitter";
  config.baseColor = glm::vec3(0.7f, 0.6f, 0.5f);
  config.tipColor = glm::vec3(0.5f, 0.4f, 0.3f);
  config.gravity = glm::vec3(0.0f);
  config.initialVelocity = glm::vec3(1.0f, 0.5f, 0.0f);
  config.spawnRadius = 5.0f;
  config.particleScale = 0.3f;
  config.fadeIn = 0.3f;
  config.fadeOut = 0.5f;
  config.velocityRandomness = 1.0f;
  config.scaleOverLifetime = 1.2f;
  config.rotationSpeed = 0.3f;
  config.billboardMode = ParticleEmitter::BillboardMode::Spherical;
  config.colorMode = ParticleEmitter::ColorMode::Gradient;
  return createFromConfig(config);
}

inline ParticleEmitterBuilder EmitterPresets::createRain() {
  ParticleEmitterConfig config;
  config.name = "Rain Emitter";
  config.baseColor = glm::vec3(0.5f, 0.6f, 0.9f);
  config.tipColor = glm::vec3(0.7f, 0.8f, 1.0f);
  config.gravity = glm::vec3(0.0f, -25.0f, 0.0f);
  config.initialVelocity = glm::vec3(2.0f, 0.0f, 0.0f);
  config.spawnRadius = 20.0f;
  config.particleScale = 0.1f;
  config.fadeIn = 0.0f;
  config.fadeOut = 0.1f;
  config.velocityRandomness = 0.2f;
  config.scaleOverLifetime = 1.0f;
  config.rotationSpeed = 0.0f;
  config.billboardMode = ParticleEmitter::BillboardMode::Cylindrical;
  config.colorMode = ParticleEmitter::ColorMode::BaseOnly;
  return createFromConfig(config);
}

inline ParticleEmitterBuilder EmitterPresets::createSnow() {
  ParticleEmitterConfig config;
  config.name = "Snow Emitter";
  config.baseColor = glm::vec3(0.95f, 0.95f, 1.0f);
  config.tipColor = glm::vec3(1.0f, 1.0f, 1.0f);
  config.gravity = glm::vec3(0.0f, -5.0f, 0.0f);
  config.initialVelocity = glm::vec3(0.5f, 0.0f, 0.0f);
  config.spawnRadius = 20.0f;
  config.particleScale = 0.2f;
  config.fadeIn = 0.0f;
  config.fadeOut = 0.1f;
  config.velocityRandomness = 0.3f;
  config.scaleOverLifetime = 1.0f;
  config.rotationSpeed = 0.2f;
  config.billboardMode = ParticleEmitter::BillboardMode::Spherical;
  config.colorMode = ParticleEmitter::ColorMode::BaseOnly;
  return createFromConfig(config);
}