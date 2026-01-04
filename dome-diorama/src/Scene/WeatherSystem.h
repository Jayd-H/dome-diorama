#pragma once
#include <glm/glm.hpp>

#include "Particles/ParticleEmitter.h"
#include "WorldState.h"

class ParticleManager;
class MaterialManager;
class Object;
class DustEmitter;
class RainEmitter;
class SmokeEmitter;

class WeatherSystem final {
 public:
  WeatherSystem(ParticleManager* particleMgr, MaterialManager* materialMgr)
      : particleManager(particleMgr),
        materialManager(materialMgr),
        sunObject(nullptr),
        moonObject(nullptr),
        rainEmitterID(INVALID_EMITTER_ID),
        dustEmitterID(INVALID_EMITTER_ID),
        snowEmitterID(INVALID_EMITTER_ID),
        particleMaterialID(INVALID_MATERIAL_ID),
        currentWeather(WeatherState::Clear),
        lastWeatherCheck(0.0f) {}

  WeatherSystem(const WeatherSystem&) = delete;
  WeatherSystem& operator=(const WeatherSystem&) = delete;

  void init();
  void update(const WorldState& worldState, float deltaTime);
  void setSunObject(Object* sun) { sunObject = sun; }
  void setMoonObject(Object* moon) { moonObject = moon; }
  void updateCelestialBodies(const WorldState& worldState);

 private:
  ParticleManager* particleManager;
  MaterialManager* materialManager;
  Object* sunObject;
  Object* moonObject;

  EmitterID rainEmitterID;
  EmitterID dustEmitterID;
  EmitterID snowEmitterID;
  MaterialID particleMaterialID;

  WeatherState currentWeather;
  float lastWeatherCheck;

  void updateWeatherEmitters(const WorldState& worldState);
  void activateRainEmitter(float intensity, size_t particleCount);
  void activateSnowEmitter(float intensity, size_t particleCount);
  void activateDustStormEmitter();
  void deactivateAllWeatherEmitters();
};