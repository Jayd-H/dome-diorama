#pragma once
#include <memory>
#include <vector>

#include "Light.h"
#include "RenderDevice.h"

constexpr uint32_t MAX_LIGHTS = 8;

struct LightData {
  alignas(16) glm::vec3 position;
  alignas(4) int type;
  alignas(16) glm::vec3 direction;
  alignas(4) float intensity;
  alignas(16) glm::vec3 color;
  alignas(4) float constant;
  alignas(4) float linear;
  alignas(4) float quadratic;
  alignas(4) float cutOff;
  alignas(4) float outerCutOff;
};

struct LightBufferObject {
  LightData lights[MAX_LIGHTS];
  alignas(4) int numLights;
};

class LightManager {
 public:
  LightManager(RenderDevice* renderDevice);
  ~LightManager();

  void init();

  LightID addLight(const Light& light);
  Light* getLight(LightID id);
  void updateLight(LightID id, const Light& light);
  void removeLight(LightID id);

  void updateLightBuffer();

  VkBuffer getLightBuffer() const { return lightBuffer; }
  int getLightCount() const { return static_cast<int>(lights.size()); }

  void cleanup();

 private:
  RenderDevice* renderDevice;

  std::vector<std::unique_ptr<Light>> lights;

  VkBuffer lightBuffer;
  VkDeviceMemory lightBufferMemory;
  void* lightBufferMapped;

  void createLightBuffer();
};