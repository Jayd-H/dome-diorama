#include "WorldState.h"

#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

#include "Debug.h"

WorldState::WorldState()
    : currentSeason(Season::DryHot), currentWeather(WeatherState::Clear) {
  Debug::log(Debug::Category::MAIN, "WorldState: Initialized");
  Debug::log(Debug::Category::MAIN, "  - Day length: ", dayLengthInSeconds,
             "s");
  Debug::log(Debug::Category::MAIN,
             "  - Season length: ", seasonLengthInSeconds, "s");
  Debug::log(Debug::Category::MAIN, "  - Starting season: DryHot");
  Debug::log(Debug::Category::MAIN, "  - Starting weather: Clear");
}

void WorldState::update(float deltaTime) {
  updateTime(deltaTime);
  updateSeason(deltaTime);
  updateWeather(deltaTime);
  updateTemperature(deltaTime);
  updateWind(deltaTime);
}

void WorldState::updateTime(float deltaTime) {
  time.totalSeconds += deltaTime;

  float dayProgress =
      fmod(time.totalSeconds, dayLengthInSeconds) / dayLengthInSeconds;
  time.normalizedTime = dayProgress;

  float totalSecondsInDay = dayProgress * 86400.0f;
  time.hours = static_cast<int>(totalSecondsInDay / 3600.0f) % 24;
  time.minutes = static_cast<int>((totalSecondsInDay / 60.0f)) % 60;
  time.seconds = static_cast<int>(totalSecondsInDay) % 60;
}

void WorldState::updateSeason(float deltaTime) {
  seasonProgress += deltaTime;

  if (seasonProgress >= seasonLengthInSeconds) {
    seasonProgress = 0.0f;

    currentSeason =
        (currentSeason == Season::DryHot) ? Season::DryCold : Season::DryHot;

    Debug::log(Debug::Category::MAIN, "WorldState: Season changed to ",
               currentSeason == Season::DryHot ? "DryHot" : "DryCold");
  }
}

void WorldState::updateWeather(float deltaTime) {
  weatherTimer += deltaTime;

  if (weatherTimer >= nextWeatherChangeIn) {
    weatherTimer = 0.0f;
    nextWeatherChangeIn = randomFloat(20.0f, 60.0f);

    WeatherState newWeather = chooseNextWeather();
    if (newWeather != currentWeather) {
      transitionToWeather(newWeather);
    }
  }

  if (currentWeather == WeatherState::LightRain ||
      currentWeather == WeatherState::HeavyRain ||
      currentWeather == WeatherState::LightSnow ||
      currentWeather == WeatherState::HeavySnow) {
    float targetIntensity = (currentWeather == WeatherState::HeavyRain ||
                             currentWeather == WeatherState::HeavySnow)
                                ? 1.0f
                                : 0.5f;
    precipitationIntensity +=
        (targetIntensity - precipitationIntensity) * deltaTime * 2.0f;
    humidity = glm::clamp(humidity + deltaTime * 0.1f, 0.0f, 1.0f);
  } else {
    precipitationIntensity -= precipitationIntensity * deltaTime * 3.0f;
    humidity = glm::clamp(humidity - deltaTime * 0.05f, 0.3f, 1.0f);
  }
}

void WorldState::updateTemperature(float deltaTime) {
  float dayNightTemp = calculateDayNightTemperature();
  float seasonalTemp = calculateSeasonalTemperature();

  float targetTemp = baseTemperature + dayNightTemp + seasonalTemp;

  if (currentWeather == WeatherState::LightRain ||
      currentWeather == WeatherState::HeavyRain) {
    targetTemp -= 5.0f;
  } else if (currentWeather == WeatherState::LightSnow ||
             currentWeather == WeatherState::HeavySnow) {
    targetTemp -= 15.0f;
  }

  currentTemperature += (targetTemp - currentTemperature) * deltaTime * 0.5f;
}

void WorldState::updateWind(float deltaTime) {
  windGustTimer += deltaTime;

  if (windGustTimer >= 5.0f) {
    windGustTimer = 0.0f;

    float gustStrength = randomFloat(0.5f, 2.0f);
    baseWindSpeed = randomFloat(1.0f, 4.0f);

    float angle = randomFloat(0.0f, glm::two_pi<float>());
    windDirection = glm::normalize(
        glm::vec3(cos(angle), randomFloat(-0.2f, 0.2f), sin(angle)));
  }

  float targetWindSpeed = baseWindSpeed;

  if (currentWeather == WeatherState::DustStorm) {
    targetWindSpeed *= 4.0f;
  } else if (currentWeather == WeatherState::HeavyRain ||
             currentWeather == WeatherState::HeavySnow) {
    targetWindSpeed *= 2.0f;
  }

  windSpeed += (targetWindSpeed - windSpeed) * deltaTime * 1.5f;
}

float WorldState::calculateDayNightTemperature() const {
  float dayNightCycle = sin(time.normalizedTime * glm::two_pi<float>());
  return dayNightCycle * 15.0f;
}

float WorldState::calculateSeasonalTemperature() const {
  if (currentSeason == Season::DryHot) {
    return 20.0f;
  } else {
    return -10.0f;
  }
}

void WorldState::transitionToWeather(WeatherState newWeather) {
  Debug::log(Debug::Category::MAIN, "WorldState: Weather transitioning from ",
             static_cast<int>(currentWeather), " to ",
             static_cast<int>(newWeather));

  currentWeather = newWeather;
}

WeatherState WorldState::chooseNextWeather() const {
  float roll = randomFloat(0.0f, 1.0f);

  if (currentSeason == Season::DryHot) {
    if (roll < 0.6f) return WeatherState::Clear;
    if (roll < 0.75f) return WeatherState::Cloudy;
    if (roll < 0.85f) return WeatherState::DustStorm;
    if (roll < 0.95f) return WeatherState::LightRain;
    return WeatherState::HeavyRain;

  } else {
    if (roll < 0.4f) return WeatherState::Clear;
    if (roll < 0.6f) return WeatherState::Cloudy;
    if (roll < 0.75f) return WeatherState::LightSnow;
    if (roll < 0.85f) return WeatherState::HeavySnow;
    if (roll < 0.92f) return WeatherState::LightRain;
    return WeatherState::HeavyRain;
  }
}

glm::vec3 WorldState::getSunDirection() const {
  float angle = time.normalizedTime * glm::two_pi<float>();

  return glm::normalize(glm::vec3(cos(angle), sin(angle), 0.2f));
}

glm::vec3 WorldState::getMoonDirection() const {
  float angle = (time.normalizedTime + 0.5f) * glm::two_pi<float>();

  return glm::normalize(glm::vec3(cos(angle), sin(angle), 0.2f));
}

float WorldState::getSunIntensity() const {
  float intensity = glm::max(0.0f, getSunDirection().y);

  if (currentWeather == WeatherState::Cloudy) {
    intensity *= 0.6f;
  } else if (currentWeather == WeatherState::HeavyRain ||
             currentWeather == WeatherState::HeavySnow) {
    intensity *= 0.3f;
  }

  return intensity;
}

float WorldState::getMoonIntensity() const {
  float intensity = glm::max(0.0f, getMoonDirection().y) * 0.2f;

  if (currentWeather == WeatherState::Cloudy) {
    intensity *= 0.5f;
  } else if (currentWeather == WeatherState::HeavyRain ||
             currentWeather == WeatherState::HeavySnow) {
    intensity *= 0.2f;
  }

  return intensity;
}

float WorldState::randomFloat(float min, float max) const {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(min, max);
  return dis(gen);
}