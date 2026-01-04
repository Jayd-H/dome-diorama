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

class EmitterPresets final {
 public:
  static ParticleEmitterBuilder createFire();
  static ParticleEmitterBuilder createSmoke();
  static ParticleEmitterBuilder createDust();
  static ParticleEmitterBuilder createRain();
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

inline ParticleEmitterBuilder EmitterPresets::createFire() {
  ParticleEmitterBuilder builder;
  builder.name("Fire Emitter")
      .baseColor(glm::vec3(1.0f, 0.9f, 0.1f))
      .tipColor(glm::vec3(1.0f, 0.3f, 0.0f))
      .gravity(glm::vec3(0.0f))
      .initialVelocity(glm::vec3(0.0f, 2.0f, 0.0f))
      .spawnRadius(0.2f)
      .particleScale(0.5f)
      .fadeTimings(0.1f, 0.5f)
      .velocityRandomness(0.5f)
      .scaleOverLifetime(0.3f)
      .rotationSpeed(1.0f)
      .billboardMode(ParticleEmitter::BillboardMode::Spherical)
      .colorMode(ParticleEmitter::ColorMode::Gradient);
  return builder;
}

inline ParticleEmitterBuilder EmitterPresets::createSmoke() {
  ParticleEmitterBuilder builder;
  builder.name("Smoke Emitter")
      .baseColor(glm::vec3(0.3f, 0.3f, 0.3f))
      .tipColor(glm::vec3(0.6f, 0.6f, 0.6f))
      .gravity(glm::vec3(0.0f))
      .initialVelocity(glm::vec3(0.0f, 1.5f, 0.0f))
      .spawnRadius(0.3f)
      .particleScale(1.5f)
      .fadeTimings(0.2f, 0.8f)
      .velocityRandomness(0.8f)
      .scaleOverLifetime(2.5f)
      .rotationSpeed(0.5f)
      .billboardMode(ParticleEmitter::BillboardMode::Spherical)
      .colorMode(ParticleEmitter::ColorMode::Gradient);
  return builder;
}

inline ParticleEmitterBuilder EmitterPresets::createDust() {
  ParticleEmitterBuilder builder;
  builder.name("Dust Emitter")
      .baseColor(glm::vec3(0.7f, 0.6f, 0.5f))
      .tipColor(glm::vec3(0.5f, 0.4f, 0.3f))
      .gravity(glm::vec3(0.0f))
      .initialVelocity(glm::vec3(1.0f, 0.5f, 0.0f))
      .spawnRadius(5.0f)
      .particleScale(0.3f)
      .fadeTimings(0.3f, 0.5f)
      .velocityRandomness(1.0f)
      .scaleOverLifetime(1.2f)
      .rotationSpeed(0.3f)
      .billboardMode(ParticleEmitter::BillboardMode::Spherical)
      .colorMode(ParticleEmitter::ColorMode::Gradient);
  return builder;
}

inline ParticleEmitterBuilder EmitterPresets::createRain() {
  ParticleEmitterBuilder builder;
  builder.name("Rain Emitter")
      .baseColor(glm::vec3(0.6f, 0.7f, 0.9f))
      .tipColor(glm::vec3(0.8f, 0.9f, 1.0f))
      .gravity(glm::vec3(0.0f, -15.0f, 0.0f))
      .initialVelocity(glm::vec3(2.0f, 0.0f, 0.0f))
      .spawnRadius(20.0f)
      .particleScale(0.1f)
      .fadeTimings(0.0f, 0.1f)
      .velocityRandomness(0.2f)
      .scaleOverLifetime(1.0f)
      .rotationSpeed(0.0f)
      .billboardMode(ParticleEmitter::BillboardMode::Cylindrical)
      .colorMode(ParticleEmitter::ColorMode::BaseOnly);
  return builder;
}