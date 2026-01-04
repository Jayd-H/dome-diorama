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

void WeatherSystem::updateWeatherEmitters(const WorldState& worldState) {
  const WeatherState newWeather = worldState.getWeather();

  if (newWeather == currentWeather) {
    return;
  }

  deactivateAllWeatherEmitters();

  currentWeather = newWeather;
  const float temperature = worldState.getTemperature();

  switch (currentWeather) {
    case WeatherState::LightRain:
      if (temperature <= 0.0f) {
        activateSnowEmitter(0.4f, 10000);
      } else {
        activateRainEmitter(0.4f, 10000);
      }
      break;
    case WeatherState::HeavyRain:
      if (temperature <= 0.0f) {
        activateSnowEmitter(0.8f, 20000);
      } else {
        activateRainEmitter(0.8f, 20000);
      }
      break;
    case WeatherState::LightSnow:
      activateSnowEmitter(0.3f, 10000);
      break;
    case WeatherState::HeavySnow:
      activateSnowEmitter(0.6f, 20000);
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
    ParticleEmitterBuilder builder;
    EmitterPresets::createRain(builder);

    ParticleEmitter* const rainEmitter =
        builder.name("Weather Rain")
            .position(0.0f, 150.0f, 0.0f)
            .maxParticles(particleCount)
            .particleLifetime(8.0f)
            .material(particleMaterialID)
            .baseColor(glm::vec3(0.5f, 0.6f, 0.9f))
            .tipColor(glm::vec3(0.7f, 0.8f, 1.0f))
            .gravity(glm::vec3(0.0f, -25.0f * intensity, 0.0f))
            .initialVelocity(glm::vec3(2.0f, 0.0f, 0.0f))
            .spawnRadius(300.0f)
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
    ParticleEmitterBuilder builder;
    EmitterPresets::createSnow(builder);

    ParticleEmitter* const snowEmitter =
        builder.name("Weather Snow")
            .position(0.0f, 150.0f, 0.0f)
            .maxParticles(particleCount)
            .particleLifetime(15.0f)
            .material(particleMaterialID)
            .baseColor(glm::vec3(0.95f, 0.95f, 1.0f))
            .tipColor(glm::vec3(1.0f, 1.0f, 1.0f))
            .gravity(glm::vec3(0.0f, -5.0f * intensity, 0.0f))
            .initialVelocity(glm::vec3(0.5f, 0.0f, 0.0f))
            .spawnRadius(300.0f)
            .particleScale(0.2f)
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
    ParticleEmitterBuilder builder;
    EmitterPresets::createDust(builder);

    ParticleEmitter* const dustEmitter =
        builder.name("Weather Dust Storm")
            .position(0.0f, 80.0f, 0.0f)
            .maxParticles(2500)
            .particleLifetime(10.0f)
            .material(particleMaterialID)
            .baseColor(glm::vec3(0.7f, 0.6f, 0.5f))
            .tipColor(glm::vec3(0.5f, 0.4f, 0.3f))
            .initialVelocity(glm::vec3(3.0f, 2.0f, 0.0f))
            .spawnRadius(300.0f)
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