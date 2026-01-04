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

  ParticleEmitterConfig(const char* inName, const glm::vec3& inBaseColor,
                        const glm::vec3& inTipColor, const glm::vec3& inGravity,
                        const glm::vec3& inInitialVelocity, float inSpawnRadius,
                        float inParticleScale, float inFadeIn, float inFadeOut,
                        float inVelocityRandomness, float inScaleOverLifetime,
                        float inRotationSpeed,
                        ParticleEmitter::BillboardMode inBillboardMode,
                        ParticleEmitter::ColorMode inColorMode)
      : baseColor(inBaseColor),
        tipColor(inTipColor),
        gravity(inGravity),
        initialVelocity(inInitialVelocity),
        name(inName),
        spawnRadius(inSpawnRadius),
        particleScale(inParticleScale),
        fadeIn(inFadeIn),
        fadeOut(inFadeOut),
        velocityRandomness(inVelocityRandomness),
        scaleOverLifetime(inScaleOverLifetime),
        rotationSpeed(inRotationSpeed),
        billboardMode(inBillboardMode),
        colorMode(inColorMode) {}

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
  static void createFire(ParticleEmitterBuilder& outBuilder);
  static void createSmoke(ParticleEmitterBuilder& outBuilder);
  static void createDust(ParticleEmitterBuilder& outBuilder);
  static void createRain(ParticleEmitterBuilder& outBuilder);
  static void createSnow(ParticleEmitterBuilder& outBuilder);

 private:
  static void createFromConfig(const ParticleEmitterConfig& config,
                               ParticleEmitterBuilder& outBuilder);
};

inline void EmitterPresets::createFromConfig(
    const ParticleEmitterConfig& config, ParticleEmitterBuilder& outBuilder) {
  outBuilder.name(config.name)
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
}

inline void EmitterPresets::createFire(ParticleEmitterBuilder& outBuilder) {
  createFromConfig(
      {"Fire Emitter", glm::vec3(1.0f, 0.9f, 0.1f), glm::vec3(1.0f, 0.3f, 0.0f),
       glm::vec3(0.0f), glm::vec3(0.0f, 2.0f, 0.0f), 0.2f, 0.5f, 0.1f, 0.5f,
       0.5f, 0.3f, 1.0f, ParticleEmitter::BillboardMode::Spherical,
       ParticleEmitter::ColorMode::Gradient},
      outBuilder);
}

inline void EmitterPresets::createSmoke(ParticleEmitterBuilder& outBuilder) {
  createFromConfig({"Smoke Emitter", glm::vec3(0.3f, 0.3f, 0.3f),
                    glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(0.0f),
                    glm::vec3(0.0f, 1.5f, 0.0f), 0.3f, 1.5f, 0.2f, 0.8f, 0.8f,
                    2.5f, 0.5f, ParticleEmitter::BillboardMode::Spherical,
                    ParticleEmitter::ColorMode::Gradient},
                   outBuilder);
}

inline void EmitterPresets::createDust(ParticleEmitterBuilder& outBuilder) {
  createFromConfig(
      {"Dust Emitter", glm::vec3(0.7f, 0.6f, 0.5f), glm::vec3(0.5f, 0.4f, 0.3f),
       glm::vec3(0.0f), glm::vec3(1.0f, 0.5f, 0.0f), 5.0f, 0.3f, 0.3f, 0.5f,
       1.0f, 1.2f, 0.3f, ParticleEmitter::BillboardMode::Spherical,
       ParticleEmitter::ColorMode::Gradient},
      outBuilder);
}

inline void EmitterPresets::createRain(ParticleEmitterBuilder& outBuilder) {
  createFromConfig({"Rain Emitter", glm::vec3(0.5f, 0.6f, 0.9f),
                    glm::vec3(0.7f, 0.8f, 1.0f), glm::vec3(0.0f, -25.0f, 0.0f),
                    glm::vec3(2.0f, 0.0f, 0.0f), 20.0f, 0.1f, 0.0f, 0.1f, 0.2f,
                    1.0f, 0.0f, ParticleEmitter::BillboardMode::Cylindrical,
                    ParticleEmitter::ColorMode::BaseOnly},
                   outBuilder);
}

inline void EmitterPresets::createSnow(ParticleEmitterBuilder& outBuilder) {
  createFromConfig({"Snow Emitter", glm::vec3(0.95f, 0.95f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, -5.0f, 0.0f),
                    glm::vec3(0.5f, 0.0f, 0.0f), 20.0f, 0.2f, 0.0f, 0.1f, 0.3f,
                    1.0f, 0.2f, ParticleEmitter::BillboardMode::Spherical,
                    ParticleEmitter::ColorMode::BaseOnly},
                   outBuilder);
}