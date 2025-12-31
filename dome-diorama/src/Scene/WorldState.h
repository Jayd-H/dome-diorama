#pragma once
#include <glm/glm.hpp>

enum class Season { DryHot, DryCold };

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

  bool isDay() const { return hours >= 6 && hours < 18; }
  bool isNight() const { return !isDay(); }
  bool isDusk() const { return hours >= 17 && hours < 19; }
  bool isDawn() const { return hours >= 5 && hours < 7; }
};

class WorldState final {
 public:
  WorldState();

  void update(float deltaTime);

  float getTemperature() const { return currentTemperature; }
  glm::vec3 getWindDirection() const { return windDirection; }
  float getWindSpeed() const { return windSpeed; }
  const TimeOfDay& getTime() const { return time; }
  Season getSeason() const { return currentSeason; }
  WeatherState getWeather() const { return currentWeather; }
  float getPrecipitationIntensity() const { return precipitationIntensity; }
  float getHumidity() const { return humidity; }

  glm::vec3 getSunDirection() const;
  glm::vec3 getMoonDirection() const;
  float getSunIntensity() const;
  float getMoonIntensity() const;

 private:
  glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);

  TimeOfDay time;

  float dayLengthInSeconds = 120.0f;
  float seasonProgress = 0.0f;
  float seasonLengthInSeconds = 300.0f;
  float weatherTimer = 0.0f;
  float nextWeatherChangeIn = 30.0f;
  float currentTemperature = 20.0f;
  float baseTemperature = 20.0f;
  float windSpeed = 2.0f;
  float windGustTimer = 0.0f;
  float baseWindSpeed = 2.0f;
  float precipitationIntensity = 0.0f;
  float humidity = 0.3f;

  Season currentSeason;
  WeatherState currentWeather;

  void updateTime(float deltaTime);
  void updateTemperature(float deltaTime);
  void updateWind(float deltaTime);
  void updateSeason(float deltaTime);
  void updateWeather(float deltaTime);
  float calculateDayNightTemperature() const;
  float calculateSeasonalTemperature() const;
  void transitionToWeather(WeatherState newWeather);
  WeatherState chooseNextWeather() const;
  float randomFloat(float min, float max) const;
};