#pragma once
#include <array>
#include <memory>
#include <vector>

#include "Light.h"
#include "Rendering/RenderDevice.h"
#include "Shadow.h"

constexpr uint32_t MAX_LIGHTS = 8;

struct LightData {
  alignas(16) glm::mat4 lightSpaceMatrix;
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
  alignas(4) int castsShadows;
  alignas(4) int shadowMapIndex;
  alignas(4) float padding;
};

struct LightBufferObject {
  std::array<LightData, MAX_LIGHTS> lights;
  alignas(4) int numLights;
  alignas(4) int numShadowMaps;
  alignas(8) float padding[2];
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

VkDescriptorSetLayout getShadowDescriptorSetLayout() const {
    return shadowSystem->getShadowDescriptorSetLayout();
  }

  VkDescriptorSet getShadowDescriptorSet() const {
    return shadowSystem->getShadowDescriptorSet();
  }

  const std::vector<ShadowMapData>& getShadowMaps() const {
    return shadowSystem->getShadowMaps();
  }

  int getShadowMapCount() const {
    return static_cast<int>(shadowSystem->getShadowMaps().size());
  }

  ShadowSystem* getShadowSystem() const { return shadowSystem.get(); }

  void cleanup();

 private:
  std::vector<std::unique_ptr<Light>> lights;
  RenderDevice* renderDevice;

  void* lightBufferMapped;
  VkBuffer lightBuffer;
  VkDeviceMemory lightBufferMemory;

  std::unique_ptr<ShadowSystem> shadowSystem;

  void createLightBuffer();
};