#pragma once
#include "ParticleEmitter.h"

class FireEmitter : public ParticleEmitter {
 public:
  FireEmitter();

  void setWaveFrequency(float frequency);
  void setWaveAmplitude(float amplitude);
  void setBaseColor(const glm::vec3& color);
  void setTipColor(const glm::vec3& color);
  void setUpwardSpeed(float speed);
  void setSpawnRadius(float radius);
  void setParticleScale(float scale);

 protected:
  void updateShaderParams() override;

 private:
  float waveFrequency;
  float waveAmplitude;
  float upwardSpeed;
  float spawnRadius;
  float particleScale;
  glm::vec3 baseColor;
  glm::vec3 tipColor;
};

class FireEmitterBuilder : public ParticleEmitterBuilder {
 public:
  FireEmitterBuilder();

  FireEmitterBuilder& name(const std::string& name);
  FireEmitterBuilder& position(const glm::vec3& pos);
  FireEmitterBuilder& position(float x, float y, float z);
  FireEmitterBuilder& maxParticles(size_t count);
  FireEmitterBuilder& particleLifetime(float lifetime);
  FireEmitterBuilder& material(MaterialID id);
  FireEmitterBuilder& active(bool isActive);

  FireEmitterBuilder& waveFrequency(float frequency);
  FireEmitterBuilder& waveAmplitude(float amplitude);
  FireEmitterBuilder& baseColor(const glm::vec3& color);
  FireEmitterBuilder& baseColor(float r, float g, float b);
  FireEmitterBuilder& tipColor(const glm::vec3& color);
  FireEmitterBuilder& tipColor(float r, float g, float b);
  FireEmitterBuilder& upwardSpeed(float speed);
  FireEmitterBuilder& spawnRadius(float radius);
  FireEmitterBuilder& particleScale(float scale);

  FireEmitter* build();

 private:
  FireEmitter* fireEmitter;
};