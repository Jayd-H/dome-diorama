#pragma once
#include <memory>
#include <utility>

#include "ParticleEmitter.h"

class ParticleEmitterBuilder final {
 public:
  ParticleEmitterBuilder();
  ~ParticleEmitterBuilder();

  ParticleEmitterBuilder(ParticleEmitterBuilder&& other) noexcept;
  ParticleEmitterBuilder& operator=(ParticleEmitterBuilder&& other) noexcept;

  ParticleEmitterBuilder(const ParticleEmitterBuilder&) = delete;
  ParticleEmitterBuilder& operator=(const ParticleEmitterBuilder&) = delete;

  ParticleEmitterBuilder& name(const std::string& n);
  ParticleEmitterBuilder& position(const glm::vec3& pos);
  ParticleEmitterBuilder& position(float x, float y, float z);
  ParticleEmitterBuilder& maxParticles(size_t count);
  ParticleEmitterBuilder& particleLifetime(float lifetime);
  ParticleEmitterBuilder& material(MaterialID id);
  ParticleEmitterBuilder& active(bool isActive);

  ParticleEmitterBuilder& baseColor(const glm::vec3& color);
  ParticleEmitterBuilder& tipColor(const glm::vec3& color);
  ParticleEmitterBuilder& gravity(const glm::vec3& g);
  ParticleEmitterBuilder& initialVelocity(const glm::vec3& v);
  ParticleEmitterBuilder& velocityRandomness(float r);
  ParticleEmitterBuilder& spawnRadius(float radius);
  ParticleEmitterBuilder& particleScale(float scale);
  ParticleEmitterBuilder& scaleOverLifetime(float scale);
  ParticleEmitterBuilder& rotationSpeed(float speed);
  ParticleEmitterBuilder& fadeTimings(float in, float out);
  ParticleEmitterBuilder& billboardMode(ParticleEmitter::BillboardMode mode);
  ParticleEmitterBuilder& colorMode(ParticleEmitter::ColorMode mode);
  ParticleEmitterBuilder& windInfluence(float influence);

  ParticleEmitter* build();

 private:
  std::unique_ptr<ParticleEmitter> emitter;
};

struct ParticleEmitterConfig {
  const char* name;
  glm::vec3 baseColor;
  glm::vec3 tipColor;
  glm::vec3 gravity;
  glm::vec3 initialVelocity;
  float spawnRadius;
  float particleScale;
  float fadeIn;
  float fadeOut;
  float velocityRandomness;
  float scaleOverLifetime;
  float rotationSpeed;
  ParticleEmitter::BillboardMode billboardMode;
  ParticleEmitter::ColorMode colorMode;
};

class EmitterPresets final {
 public:
  static ParticleEmitterBuilder createFire();
  static ParticleEmitterBuilder createSmoke();
  static ParticleEmitterBuilder createDust();
  static ParticleEmitterBuilder createRain();

 private:
  static ParticleEmitterBuilder createFromConfig(
      const ParticleEmitterConfig& config);
};

inline ParticleEmitterBuilder::ParticleEmitterBuilder()
    : emitter(std::make_unique<ParticleEmitter>()) {}

inline ParticleEmitterBuilder::~ParticleEmitterBuilder() = default;

inline ParticleEmitterBuilder::ParticleEmitterBuilder(
    ParticleEmitterBuilder&& other) noexcept
    : emitter(std::move(other.emitter)) {}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::operator=(
    ParticleEmitterBuilder&& other) noexcept {
  if (this != &other) {
    emitter = std::move(other.emitter);
  }
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::name(
    const std::string& n) {
  if (emitter) emitter->setName(n);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::position(
    const glm::vec3& pos) {
  if (emitter) emitter->setPosition(pos);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::position(float x,
                                                                float y,
                                                                float z) {
  if (emitter) emitter->setPosition(glm::vec3(x, y, z));
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::maxParticles(
    size_t count) {
  if (emitter) emitter->setMaxParticles(count);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::particleLifetime(
    float lifetime) {
  if (emitter) emitter->setParticleLifetime(lifetime);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::material(MaterialID id) {
  if (emitter) emitter->setMaterialID(id);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::active(bool isActive) {
  if (emitter) emitter->setActive(isActive);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::baseColor(
    const glm::vec3& color) {
  if (emitter) emitter->setBaseColor(color);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::tipColor(
    const glm::vec3& color) {
  if (emitter) emitter->setTipColor(color);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::gravity(
    const glm::vec3& g) {
  if (emitter) emitter->setGravity(g);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::initialVelocity(
    const glm::vec3& v) {
  if (emitter) emitter->setInitialVelocity(v);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::velocityRandomness(
    float r) {
  if (emitter) emitter->setVelocityRandomness(r);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::spawnRadius(
    float radius) {
  if (emitter) emitter->setSpawnRadius(radius);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::particleScale(
    float scale) {
  if (emitter) emitter->setParticleScale(scale);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::scaleOverLifetime(
    float scale) {
  if (emitter) emitter->setScaleOverLifetime(scale);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::rotationSpeed(
    float speed) {
  if (emitter) emitter->setRotationSpeed(speed);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::fadeTimings(float in,
                                                                   float out) {
  if (emitter) {
    emitter->setFadeInDuration(in);
    emitter->setFadeOutDuration(out);
  }
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::billboardMode(
    ParticleEmitter::BillboardMode mode) {
  if (emitter) emitter->setBillboardMode(mode);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::colorMode(
    ParticleEmitter::ColorMode mode) {
  if (emitter) emitter->setColorMode(mode);
  return *this;
}

inline ParticleEmitterBuilder& ParticleEmitterBuilder::windInfluence(
    float influence) {
  if (emitter) emitter->setWindInfluence(influence);
  return *this;
}

inline ParticleEmitter* ParticleEmitterBuilder::build() {
  return emitter.release();
}

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
  return createFromConfig(
      {"Fire Emitter", glm::vec3(1.0f, 0.9f, 0.1f), glm::vec3(1.0f, 0.3f, 0.0f),
       glm::vec3(0.0f), glm::vec3(0.0f, 2.0f, 0.0f), 0.2f, 0.5f, 0.1f, 0.5f,
       0.5f, 0.3f, 1.0f, ParticleEmitter::BillboardMode::Spherical,
       ParticleEmitter::ColorMode::Gradient});
}

inline ParticleEmitterBuilder EmitterPresets::createSmoke() {
  return createFromConfig({"Smoke Emitter", glm::vec3(0.3f, 0.3f, 0.3f),
                           glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(0.0f),
                           glm::vec3(0.0f, 1.5f, 0.0f), 0.3f, 1.5f, 0.2f, 0.8f,
                           0.8f, 2.5f, 0.5f,
                           ParticleEmitter::BillboardMode::Spherical,
                           ParticleEmitter::ColorMode::Gradient});
}

inline ParticleEmitterBuilder EmitterPresets::createDust() {
  return createFromConfig(
      {"Dust Emitter", glm::vec3(0.7f, 0.6f, 0.5f), glm::vec3(0.5f, 0.4f, 0.3f),
       glm::vec3(0.0f), glm::vec3(1.0f, 0.5f, 0.0f), 5.0f, 0.3f, 0.3f, 0.5f,
       1.0f, 1.2f, 0.3f, ParticleEmitter::BillboardMode::Spherical,
       ParticleEmitter::ColorMode::Gradient});
}

inline ParticleEmitterBuilder EmitterPresets::createRain() {
  return createFromConfig(
      {"Rain Emitter", glm::vec3(0.6f, 0.7f, 0.9f), glm::vec3(0.8f, 0.9f, 1.0f),
       glm::vec3(0.0f, -15.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f), 20.0f, 0.1f,
       0.0f, 0.1f, 0.2f, 1.0f, 0.0f,
       ParticleEmitter::BillboardMode::Cylindrical,
       ParticleEmitter::ColorMode::BaseOnly});
}