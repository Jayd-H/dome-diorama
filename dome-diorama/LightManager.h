#pragma once
#include <array>
#include <memory>
#include <vector>

#include "Light.h"
#include "RenderDevice.h"

constexpr uint32_t MAX_LIGHTS = 8;
constexpr uint32_t SHADOW_MAP_SIZE = 4096;

struct LightData {
  alignas(16) glm::mat4 lightSpaceMatrix;
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 direction;
  alignas(16) glm::vec3 color;
  alignas(4) int type;
  alignas(4) float intensity;
  alignas(4) float constant;
  alignas(4) float linear;
  alignas(4) float quadratic;
  alignas(4) float cutOff;
  alignas(4) float outerCutOff;
  alignas(4) int castsShadows;
  alignas(4) int shadowMapIndex;
};

struct LightBufferObject {
  std::array<LightData, MAX_LIGHTS> lights;
  alignas(4) int numLights;
};

class LightManager final {
 public:
  explicit LightManager(RenderDevice* renderDevice);
  ~LightManager();

  LightManager(const LightManager&) = delete;
  LightManager& operator=(const LightManager&) = delete;

  void init();
  LightID addLight(const Light& light);
  Light* getLight(LightID id);
  void updateLight(LightID id, const Light& light);
  void removeLight(LightID id);
  void updateLightBuffer();
  void updateLightSpaceMatrix(LightID id, const glm::mat4& matrix);
  VkBuffer getLightBuffer() const { return lightBuffer; }
  int getLightCount() const { return static_cast<int>(lights.size()); }
  void createShadowResources();
  void cleanupShadowResources();
  VkDescriptorSetLayout getShadowDescriptorSetLayout() const {
    return shadowDescriptorSetLayout;
  }
  VkDescriptorSet getShadowDescriptorSet() const { return shadowDescriptorSet; }
  void cleanup();

 private:
  std::vector<std::unique_ptr<Light>> lights;
  RenderDevice* renderDevice;
  void* lightBufferMapped;
  VkBuffer lightBuffer;
  VkDeviceMemory lightBufferMemory;
  VkDescriptorSetLayout shadowDescriptorSetLayout;
  VkDescriptorPool shadowDescriptorPool;
  VkDescriptorSet shadowDescriptorSet;

  void createLightBuffer();
  void createShadowMapForLight(Light* light);
  void createShadowDescriptorSetLayout();
  void createShadowDescriptorPool();
  void createShadowDescriptorSet();
  void updateShadowDescriptorSet();
};