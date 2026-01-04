#include "WeatherSystem.h"

#include "Particles/EmitterTypes.h"
#include "Particles/ParticleManager.h"
#include "Resources/MaterialManager.h"
#include "Resources/Object.h"
#include "Util/Debug.h"

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
    const float sunOrbitRadius = 100.0f;
    const glm::vec3 sunPos = -sunDir * sunOrbitRadius;
    sunObject->setPosition(sunPos);

    Debug::log(Debug::Category::WORLD, "Sun position: (", sunPos.x, ", ",
               sunPos.y, ", ", sunPos.z, ")");
  }

  if (moonObject) {
    const glm::vec3 moonDir = worldState.getMoonDirection();
    const float moonOrbitRadius = 100.0f;
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
    ParticleEmitter* const rainEmitter =
        EmitterPresets::createRain()
            .name("Weather Rain")
            .position(0.0f, 50.0f, 0.0f)
            .maxParticles(particleCount)
            .particleLifetime(2.5f)
            .material(particleMaterialID)
            .baseColor(glm::vec3(0.6f, 0.7f, 0.9f))
            .tipColor(glm::vec3(0.8f, 0.9f, 1.0f))
            .gravity(glm::vec3(0.0f, -15.0f * intensity, 0.0f))
            .initialVelocity(glm::vec3(2.0f, 0.0f, 0.0f))
            .spawnRadius(80.0f)
            .particleScale(0.1f)
            .windInfluence(0.7f)
            .build();

    rainEmitterID = particleManager->registerEmitter(rainEmitter);
  } else {
    ParticleEmitter* const emitter = particleManager->getEmitter(rainEmitterID);
    if (emitter) {
      emitter->setActive(true);
    }
  }
}

void WeatherSystem::activateSnowEmitter(float intensity, size_t particleCount) {
  if (snowEmitterID == INVALID_EMITTER_ID) {
    ParticleEmitter* const snowEmitter =
        EmitterPresets::createRain()
            .name("Weather Snow")
            .position(0.0f, 50.0f, 0.0f)
            .maxParticles(particleCount)
            .particleLifetime(4.0f)
            .material(particleMaterialID)
            .baseColor(glm::vec3(0.95f, 0.95f, 1.0f))
            .tipColor(glm::vec3(1.0f, 1.0f, 1.0f))
            .gravity(glm::vec3(0.0f, -3.0f * intensity, 0.0f))
            .initialVelocity(glm::vec3(1.5f, 0.0f, 0.0f))
            .spawnRadius(80.0f)
            .particleScale(0.15f)
            .windInfluence(0.9f)
            .build();

    snowEmitterID = particleManager->registerEmitter(snowEmitter);
  } else {
    ParticleEmitter* const emitter = particleManager->getEmitter(snowEmitterID);
    if (emitter) {
      emitter->setActive(true);
    }
  }
}

void WeatherSystem::activateDustStormEmitter() {
  if (dustEmitterID == INVALID_EMITTER_ID) {
    ParticleEmitter* const dustEmitter =
        EmitterPresets::createDust()
            .name("Weather Dust Storm")
            .position(0.0f, 5.0f, 0.0f)
            .maxParticles(1500)
            .particleLifetime(5.0f)
            .material(particleMaterialID)
            .baseColor(glm::vec3(0.7f, 0.6f, 0.5f))
            .tipColor(glm::vec3(0.5f, 0.4f, 0.3f))
            .initialVelocity(glm::vec3(3.0f, 2.0f, 0.0f))
            .spawnRadius(100.0f)
            .particleScale(0.5f)
            .windInfluence(1.0f)
            .build();

    dustEmitterID = particleManager->registerEmitter(dustEmitter);
  } else {
    ParticleEmitter* const emitter = particleManager->getEmitter(dustEmitterID);
    if (emitter) {
      emitter->setActive(true);
    }
  }
}

void WeatherSystem::deactivateAllWeatherEmitters() {
  if (rainEmitterID != INVALID_EMITTER_ID) {
    ParticleEmitter* const emitter = particleManager->getEmitter(rainEmitterID);
    if (emitter) {
      emitter->setActive(false);
    }
  }

  if (snowEmitterID != INVALID_EMITTER_ID) {
    ParticleEmitter* const emitter = particleManager->getEmitter(snowEmitterID);
    if (emitter) {
      emitter->setActive(false);
    }
  }

  if (dustEmitterID != INVALID_EMITTER_ID) {
    ParticleEmitter* const emitter = particleManager->getEmitter(dustEmitterID);
    if (emitter) {
      emitter->setActive(false);
    }
  }
}