#include "Particles/ParticleEmitter.h"

ParticleEmitter::ParticleEmitter()
    : shaderParams{},
      name("Unnamed Emitter"),
      position(0.0f),
      maxParticles(1000),
      particleLifetime(2.0f),
      materialID(0),
      time(0.0f),
      active(true) {
  // Initialize Shader Params Defaults
  shaderParams.emitterPosition = position;
  shaderParams.time = 0.0f;
  shaderParams.baseColor = glm::vec3(1.0f);
  shaderParams.particleLifetime = particleLifetime;
  shaderParams.tipColor = glm::vec3(1.0f);
  shaderParams.maxParticles = static_cast<float>(maxParticles);
  shaderParams.gravity = glm::vec3(0.0f);
  shaderParams.spawnRadius = 1.0f;
  shaderParams.initialVelocity = glm::vec3(0.0f);
  shaderParams.particleScale = 1.0f;
  shaderParams.fadeInDuration = 0.1f;
  shaderParams.fadeOutDuration = 0.3f;
  shaderParams.billboardMode = static_cast<int>(BillboardMode::Spherical);
  shaderParams.colorMode = static_cast<int>(ColorMode::Gradient);
  shaderParams.velocityRandomness = 0.5f;
  shaderParams.scaleOverLifetime = 1.0f;
  shaderParams.rotationSpeed = 0.0f;
  shaderParams.windDirection = glm::vec3(0.0f);
  shaderParams.windStrength = 0.0f;
  shaderParams.windInfluence = 1.0f;

  // Zero out padding
  shaderParams.padding1 = 0.0f;
  shaderParams.padding2 = 0.0f;
}

ParticleEmitter::~ParticleEmitter() = default;