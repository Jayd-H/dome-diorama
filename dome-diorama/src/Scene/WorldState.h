#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <random>

enum class WeatherState {
  Clear,
  Cloudy,
  LightRain,
  HeavyRain,
  LightSnow,
  HeavySnow,
  DustStorm
};

struct TimeOfDay {
  float totalSeconds = 0.0f;
  float normalizedTime = 0.5f;
  int hours = 12;
  int minutes = 0;
  int seconds = 0;

  inline bool isDay() const { return hours >= 6 && hours < 18; }
  inline bool isNight() const { return !isDay(); }
  inline bool isDusk() const { return hours >= 17 && hours < 19; }
  inline bool isDawn() const { return hours >= 5 && hours < 7; }
};

struct WorldConfig {
  float dayLengthInSeconds = 120.0f;
  int startingHour = 12;
  int startingMinute = 0;

  float startingTemperature = 20.0f;
  float minTemperature = -15.0f;
  float maxTemperature = 40.0f;

  float startingHumidity = 0.3f;
  float minHumidity = 0.1f;
  float maxHumidity = 0.95f;

  float startingWindSpeed = 2.0f;
  float minWindSpeed = 0.5f;
  float maxWindSpeed = 10.0f;

  float parameterUpdateInterval = 30.0f;
  float dayNightTempVariation = 8.0f;
};

class WorldState final {
 public:
  inline WorldState(const WorldConfig& config = WorldConfig())
      : currentTemperature(config.startingTemperature),
        targetTemperature(config.startingTemperature),
        humidity(config.startingHumidity),
        targetHumidity(config.startingHumidity),
        windSpeed(config.startingWindSpeed),
        targetWindSpeed(config.startingWindSpeed),
        windDirection(1.0f, 0.0f, 0.0f),
        precipitationIntensity(0.0f),
        dayLengthInSeconds(config.dayLengthInSeconds),
        minTemperature(config.minTemperature),
        maxTemperature(config.maxTemperature),
        minHumidity(config.minHumidity),
        maxHumidity(config.maxHumidity),
        minWindSpeed(config.minWindSpeed),
        maxWindSpeed(config.maxWindSpeed),
        parameterUpdateInterval(config.parameterUpdateInterval),
        dayNightTempVariation(config.dayNightTempVariation),
        rng(std::random_device{}()) {
    time.hours = config.startingHour;
    time.minutes = config.startingMinute;
    time.normalizedTime =
        (config.startingHour * 3600.0f + config.startingMinute * 60.0f) /
        86400.0f;
    time.totalSeconds = time.normalizedTime * dayLengthInSeconds;

    updateTargets();
  }

  inline void update(float deltaTime) {
    updateTime(deltaTime);
    updateEnvironmentalParameters(deltaTime);
    currentWeather = determineWeatherFromConditions();
    updatePrecipitation(deltaTime);
  }

  inline float getTemperature() const { return currentTemperature; }
  inline glm::vec3 getWindDirection() const { return windDirection; }
  inline float getWindSpeed() const { return windSpeed; }
  inline const TimeOfDay& getTime() const { return time; }
  inline WeatherState getWeather() const { return currentWeather; }
  inline float getPrecipitationIntensity() const {
    return precipitationIntensity;
  }
  inline float getHumidity() const { return humidity; }

  inline glm::vec3 getSunDirection() const {
    const float angle = time.normalizedTime * glm::two_pi<float>();
    return glm::normalize(glm::vec3(cos(angle), sin(angle), 0.2f));
  }

  inline glm::vec3 getMoonDirection() const {
    const float angle = (time.normalizedTime + 0.5f) * glm::two_pi<float>();
    return glm::normalize(glm::vec3(cos(angle), sin(angle), 0.2f));
  }

  inline float getSunIntensity() const {
    float intensity = glm::max(0.0f, getSunDirection().y);

    if (currentWeather == WeatherState::Cloudy) {
      intensity *= 0.6f;
    } else if (currentWeather == WeatherState::HeavyRain ||
               currentWeather == WeatherState::HeavySnow) {
      intensity *= 0.3f;
    } else if (currentWeather == WeatherState::DustStorm) {
      intensity *= 0.4f;
    }

    return intensity;
  }

  inline float getMoonIntensity() const {
    float intensity = glm::max(0.0f, getMoonDirection().y) * 0.2f;

    if (currentWeather == WeatherState::Cloudy) {
      intensity *= 0.5f;
    } else if (currentWeather == WeatherState::HeavyRain ||
               currentWeather == WeatherState::HeavySnow) {
      intensity *= 0.2f;
    }

    return intensity;
  }

 private:
  TimeOfDay time;
  float dayLengthInSeconds;

  float currentTemperature;
  float targetTemperature;
  float humidity;
  float targetHumidity;
  float windSpeed;
  float targetWindSpeed;
  glm::vec3 windDirection;
  float precipitationIntensity;

  float minTemperature;
  float maxTemperature;
  float minHumidity;
  float maxHumidity;
  float minWindSpeed;
  float maxWindSpeed;
  float parameterUpdateInterval;
  float dayNightTempVariation;

  WeatherState currentWeather = WeatherState::Clear;

  float parameterUpdateTimer = 0.0f;

  mutable std::mt19937 rng;

  inline void updateTime(float deltaTime) {
    time.totalSeconds += deltaTime;

    const float dayProgress =
        fmod(time.totalSeconds, dayLengthInSeconds) / dayLengthInSeconds;
    time.normalizedTime = dayProgress;

    const float totalSecondsInDay = dayProgress * 86400.0f;
    time.hours = static_cast<int>(totalSecondsInDay / 3600.0f) % 24;
    time.minutes = static_cast<int>((totalSecondsInDay / 60.0f)) % 60;
    time.seconds = static_cast<int>(totalSecondsInDay) % 60;
  }

  inline void updateEnvironmentalParameters(float deltaTime) {
    parameterUpdateTimer += deltaTime;

    if (parameterUpdateTimer >= parameterUpdateInterval) {
      parameterUpdateTimer = 0.0f;
      updateTargets();
    }

    const float transitionSpeed = 0.3f;
    currentTemperature +=
        (targetTemperature - currentTemperature) * deltaTime * transitionSpeed;
    humidity += (targetHumidity - humidity) * deltaTime * transitionSpeed;
    windSpeed += (targetWindSpeed - windSpeed) * deltaTime * transitionSpeed;

    const float dayNightTempModifier = calculateDayNightTemperature();
    currentTemperature += dayNightTempModifier * deltaTime * 0.5f;
  }

  inline void updateTargets() {
    targetTemperature = randomFloat(minTemperature, maxTemperature);
    targetHumidity = randomFloat(minHumidity, maxHumidity);
    targetWindSpeed = randomFloat(minWindSpeed, maxWindSpeed);

    const float angle = randomFloat(0.0f, glm::two_pi<float>());
    windDirection = glm::normalize(
        glm::vec3(cos(angle), randomFloat(-0.2f, 0.2f), sin(angle)));
  }

  inline float calculateDayNightTemperature() const {
    const float dayNightCycle = sin(time.normalizedTime * glm::two_pi<float>());
    return dayNightCycle * dayNightTempVariation;
  }

  inline WeatherState determineWeatherFromConditions() const {
    const bool isCold = currentTemperature < 5.0f;
    const bool isWarm = currentTemperature > 20.0f;
    const bool isDry = humidity < 0.4f;
    const bool isHumid = humidity > 0.7f;
    const bool isWindy = windSpeed > 5.0f;
    const bool isCalm = windSpeed < 2.0f;

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const float roll = dist(rng);

    if (isWindy && isDry && isWarm) {
      return WeatherState::DustStorm;
    }

    if (isCold && isHumid) {
      if (isWindy) {
        return roll < 0.7f ? WeatherState::HeavySnow : WeatherState::LightSnow;
      }
      return roll < 0.5f ? WeatherState::LightSnow : WeatherState::Cloudy;
    }

    if (isHumid && !isCold) {
      if (isWindy) {
        return roll < 0.6f ? WeatherState::HeavyRain : WeatherState::LightRain;
      }
      if (isCalm) {
        return roll < 0.3f ? WeatherState::LightRain : WeatherState::Cloudy;
      }
      return roll < 0.4f ? WeatherState::LightRain : WeatherState::Cloudy;
    }

    if (isHumid && !isDry) {
      return roll < 0.6f ? WeatherState::Cloudy : WeatherState::Clear;
    }

    if (isDry) {
      return roll < 0.8f ? WeatherState::Clear : WeatherState::Cloudy;
    }

    return roll < 0.7f ? WeatherState::Clear : WeatherState::Cloudy;
  }

  inline void updatePrecipitation(float deltaTime) {
    float targetPrecipitation = 0.0f;

    switch (currentWeather) {
      case WeatherState::LightRain:
      case WeatherState::LightSnow:
        targetPrecipitation = 0.5f;
        break;
      case WeatherState::HeavyRain:
      case WeatherState::HeavySnow:
        targetPrecipitation = 1.0f;
        break;
      case WeatherState::DustStorm:
        targetPrecipitation = 0.3f;
        break;
      default:
        targetPrecipitation = 0.0f;
        break;
    }

    const float transitionSpeed =
        targetPrecipitation > precipitationIntensity ? 2.0f : 3.0f;
    precipitationIntensity += (targetPrecipitation - precipitationIntensity) *
                              deltaTime * transitionSpeed;
  }

  inline float randomFloat(float min, float max) const {
    std::uniform_real_distribution<float> dis(min, max);
    return dis(rng);
  }
};