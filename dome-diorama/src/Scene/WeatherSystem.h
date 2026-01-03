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
        currentWeather(WeatherState::Clear),
        rainEmitterID(INVALID_EMITTER_ID),
        dustEmitterID(INVALID_EMITTER_ID),
        snowEmitterID(INVALID_EMITTER_ID),
        particleMaterialID(INVALID_MATERIAL_ID),
        sunObject(nullptr),
        moonObject(nullptr),
        lastWeatherCheck(0.0f) {}

  void init();
  void update(const WorldState& worldState, float deltaTime);
  void setSunObject(Object* sun) { sunObject = sun; }
  void setMoonObject(Object* moon) { moonObject = moon; }
  void updateCelestialBodies(const WorldState& worldState);

 private:
  ParticleManager* particleManager;
  MaterialManager* materialManager;
  WeatherState currentWeather;
  EmitterID rainEmitterID;
  EmitterID dustEmitterID;
  EmitterID snowEmitterID;
  MaterialID particleMaterialID;
  Object* sunObject;
  Object* moonObject;
  float lastWeatherCheck;

  void updateWeatherEmitters(const WorldState& worldState);
  void activateRainEmitter(float intensity, size_t particleCount);
  void activateSnowEmitter(float intensity, size_t particleCount);
  void activateDustStormEmitter();
  void deactivateAllWeatherEmitters();
};