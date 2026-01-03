#include "PerlinNoise.h"

float PerlinNoise::octaveNoise(float x, float y, int octaves,
                               float persistence) const {
  float total = 0.0f;
  float frequency = 1.0f;
  float amplitude = 1.0f;
  float maxValue = 0.0f;

  for (int i = 0; i < octaves; i++) {
    total += noise(x * frequency, y * frequency, 0.0f) * amplitude;
    maxValue += amplitude;
    amplitude *= persistence;
    frequency *= 2.0f;
  }

  return total / maxValue;
}