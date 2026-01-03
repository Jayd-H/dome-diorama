#pragma once
#include <glm/glm.hpp>

#include "Particles/ParticleEmitter.h"

struct PlantState {
  float health = 100.0f;
  float growthProgress = 0.0f;
  float burnTimer = 0.0f;
  float waterLevel = 50.0f;
  float spreadTimer = 0.0f;
  bool isOnFire = false;
  bool isDead = false;
  EmitterID fireEmitterID = INVALID_EMITTER_ID;

  static constexpr float MAX_HEALTH = 100.0f;
  static constexpr float GROWTH_THRESHOLD = 100.0f;
  static constexpr float BURN_DAMAGE_PER_SECOND = 8.0f;
  static constexpr float MIN_BURN_STAGE_REVERT_TIME = 5.0f;
  static constexpr float MAX_BURN_STAGE_REVERT_TIME = 10.0f;
  static constexpr float HEALTH_AFTER_BURN_REVERT = 10.0f;
  static constexpr float FIRE_SPREAD_RADIUS = 5.0f;
  static constexpr float HEAT_DAMAGE_THRESHOLD = 35.0f;
  static constexpr float OPTIMAL_TEMP_MIN = 15.0f;
  static constexpr float OPTIMAL_TEMP_MAX = 30.0f;
  static constexpr float WATER_DRAIN_RATE = 2.0f;
  static constexpr float RAIN_WATER_RATE = 20.0f;
  static constexpr float SPREAD_CHECK_INTERVAL = 60.0f;
  static constexpr float MIN_SPREAD_DISTANCE = 3.0f;
  static constexpr float MAX_SPREAD_DISTANCE = 8.0f;
};

struct PlantWindData {
  alignas(16) glm::vec3 windDirection;
  alignas(4) float windStrength;
  alignas(4) float time;
  alignas(4) float swayAmount;
  alignas(4) float swaySpeed;
  alignas(4) float padding;
};