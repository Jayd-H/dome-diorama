#pragma once
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

class PerlinNoise final {
 public:
  explicit PerlinNoise(unsigned int seed = 0) {
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.end(), engine);
    p.insert(p.end(), p.begin(), p.end());
  }

  float noise(float x, float y, float z) const {
    const int X = static_cast<int>(std::floor(x)) & 255;
    const int Y = static_cast<int>(std::floor(y)) & 255;
    const int Z = static_cast<int>(std::floor(z)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    const float u = fade(x);
    const float v = fade(y);
    const float w = fade(z);

    const int A = p[X] + Y;
    const int AA = p[A] + Z;
    const int AB = p[static_cast<size_t>(A) + 1] + Z;
    const int B = p[static_cast<size_t>(X) + 1] + Y;
    const int BA = p[B] + Z;
    const int BB = p[static_cast<size_t>(B) + 1] + Z;

    return lerp(
        w,
        lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
             lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
        lerp(v,
             lerp(u, grad(p[static_cast<size_t>(AA) + 1], x, y, z - 1),
                  grad(p[static_cast<size_t>(BA) + 1], x - 1, y, z - 1)),
             lerp(u, grad(p[static_cast<size_t>(AB) + 1], x, y - 1, z - 1),
                  grad(p[static_cast<size_t>(BB) + 1], x - 1, y - 1, z - 1))));
  }

  float octaveNoise(float x, float y, int octaves, float persistence) const;

 private:
  std::vector<int> p;

  float fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
  }

  float lerp(float t, float a, float b) const { return a + t * (b - a); }

  float grad(int hash, float x, float y, float z) const {
    const int h = hash & 15;
    const float u = h < 8 ? x : y;
    const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
  }
};

inline float PerlinNoise::octaveNoise(float x, float y, int octaves,
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