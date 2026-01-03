#include "WeatherSystem.h"

#include "Particles/DustEmitter.h"
#include "Particles/ParticleManager.h"
#include "Particles/RainEmitter.h"
#include "Resources/MaterialManager.h"
#include "Resources/Object.h"

void WeatherSystem::init() {
  particleMaterialID =
      materialManager->registerMaterial(MaterialBuilder()
                                            .name("Weather Particle Material")
                                            .albedoColor(1.0f, 1.0f, 1.0f)
                                            .roughness(0.0f)
                                            .metallic(0.0f)
                                            .transparent(true));
}

void WeatherSystem::update(const WorldState& worldState, float deltaTime) {
  lastWeatherCheck += deltaTime;

  if (lastWeatherCheck >= 1.0f) {
    lastWeatherCheck = 0.0f;
    updateWeatherEmitters(worldState);
  }
}

void WeatherSystem::updateCelestialBodies(const WorldState& worldState) {
  if (sunObject) {
    const glm::vec3 sunDir = worldState.getSunDirection();
    const float sunOrbitRadius = 500.0f;
    const glm::vec3 sunPos = -sunDir * sunOrbitRadius;
    sunObject->setPosition(sunPos);

    Debug::log(Debug::Category::WORLD, "Sun position: (", sunPos.x, ", ",
               sunPos.y, ", ", sunPos.z, ")");
  }

  if (moonObject) {
    const glm::vec3 moonDir = worldState.getMoonDirection();
    const float moonOrbitRadius = 450.0f;
    const glm::vec3 moonPos = -moonDir * moonOrbitRadius;
    moonObject->setPosition(moonPos);
  }
}

void WeatherSystem::updateWeatherEmitters(const WorldState& worldState) {
  const WeatherState newWeather = worldState.getWeather();

  if (newWeather == currentWeather) {
    return;
  }

  deactivateAllWeatherEmitters();

  currentWeather = newWeather;

  switch (currentWeather) {
    case WeatherState::LightRain:
      activateRainEmitter(0.4f, 1000);
      break;
    case WeatherState::HeavyRain:
      activateRainEmitter(0.8f, 2000);
      break;
    case WeatherState::LightSnow:
      activateSnowEmitter(0.3f, 800);
      break;
    case WeatherState::HeavySnow:
      activateSnowEmitter(0.6f, 1500);
      break;
    case WeatherState::DustStorm:
      activateDustStormEmitter();
      break;
    default:
      break;
  }
}

void WeatherSystem::activateRainEmitter(float intensity, size_t particleCount) {
  if (rainEmitterID == INVALID_EMITTER_ID) {
    RainEmitter* rainEmitter = RainEmitterBuilder()
                                   .name("Weather Rain")
                                   .position(0.0f, 50.0f, 0.0f)
                                   .maxParticles(particleCount)
                                   .particleLifetime(2.5f)
                                   .material(particleMaterialID)
                                   .baseColor(0.6f, 0.7f, 0.9f)
                                   .tipColor(0.8f, 0.9f, 1.0f)
                                   .downwardSpeed(15.0f * intensity)
                                   .windStrength(2.0f)
                                   .spawnRadius(80.0f)
                                   .particleScale(0.1f)
                                   .windInfluence(0.7f)
                                   .build();

    rainEmitterID = particleManager->registerEmitter(rainEmitter);
  } else {
    ParticleEmitter* emitter = particleManager->getEmitter(rainEmitterID);
    if (emitter) {
      emitter->setActive(true);
    }
  }
}

void WeatherSystem::activateSnowEmitter(float intensity, size_t particleCount) {
  if (snowEmitterID == INVALID_EMITTER_ID) {
    RainEmitter* snowEmitter = RainEmitterBuilder()
                                   .name("Weather Snow")
                                   .position(0.0f, 50.0f, 0.0f)
                                   .maxParticles(particleCount)
                                   .particleLifetime(4.0f)
                                   .material(particleMaterialID)
                                   .baseColor(0.95f, 0.95f, 1.0f)
                                   .tipColor(1.0f, 1.0f, 1.0f)
                                   .downwardSpeed(3.0f * intensity)
                                   .windStrength(1.5f)
                                   .spawnRadius(80.0f)
                                   .particleScale(0.15f)
                                   .windInfluence(0.9f)
                                   .build();

    snowEmitterID = particleManager->registerEmitter(snowEmitter);
  } else {
    ParticleEmitter* emitter = particleManager->getEmitter(snowEmitterID);
    if (emitter) {
      emitter->setActive(true);
    }
  }
}

void WeatherSystem::activateDustStormEmitter() {
  if (dustEmitterID == INVALID_EMITTER_ID) {
    DustEmitter* dustEmitter = DustEmitterBuilder()
                                   .name("Weather Dust Storm")
                                   .position(0.0f, 5.0f, 0.0f)
                                   .maxParticles(1500)
                                   .particleLifetime(5.0f)
                                   .material(particleMaterialID)
                                   .baseColor(0.7f, 0.6f, 0.5f)
                                   .tipColor(0.5f, 0.4f, 0.3f)
                                   .swirlingSpeed(3.0f)
                                   .driftSpeed(2.0f)
                                   .spawnRadius(100.0f)
                                   .particleScale(0.5f)
                                   .windInfluence(1.0f)
                                   .build();

    dustEmitterID = particleManager->registerEmitter(dustEmitter);
  } else {
    ParticleEmitter* emitter = particleManager->getEmitter(dustEmitterID);
    if (emitter) {
      emitter->setActive(true);
    }
  }
}

void WeatherSystem::deactivateAllWeatherEmitters() {
  if (rainEmitterID != INVALID_EMITTER_ID) {
    ParticleEmitter* emitter = particleManager->getEmitter(rainEmitterID);
    if (emitter) {
      emitter->setActive(false);
    }
  }

  if (snowEmitterID != INVALID_EMITTER_ID) {
    ParticleEmitter* emitter = particleManager->getEmitter(snowEmitterID);
    if (emitter) {
      emitter->setActive(false);
    }
  }

  if (dustEmitterID != INVALID_EMITTER_ID) {
    ParticleEmitter* emitter = particleManager->getEmitter(dustEmitterID);
    if (emitter) {
      emitter->setActive(false);
    }
  }
}