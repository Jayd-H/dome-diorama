#pragma once
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

#include "Light.h"
#include "Rendering/RenderDevice.h"
#include "Shadow.h"
#include "Util/Debug.h"

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
  explicit LightManager(RenderDevice* renderDevice)
      : renderDevice(renderDevice),
        lightBuffer(VK_NULL_HANDLE),
        lightBufferMemory(VK_NULL_HANDLE),
        lightBufferMapped(nullptr),
        shadowSystem(nullptr) {
    Debug::log(Debug::Category::LIGHTS, "LightManager: Constructor called");
  }

  ~LightManager() {
    Debug::log(Debug::Category::LIGHTS, "LightManager: Destructor called");
    cleanup();
  }

  LightManager(const LightManager&) = delete;
  LightManager& operator=(const LightManager&) = delete;

  void init() {
    Debug::log(Debug::Category::LIGHTS, "LightManager: Initializing");
    createLightBuffer();
    shadowSystem = std::make_unique<ShadowSystem>(renderDevice);
    shadowSystem->init();
    Debug::log(Debug::Category::LIGHTS,
               "LightManager: Initialization complete");
  }

  LightID addLight(const Light& light) {
    if (lights.size() >= MAX_LIGHTS) {
      Debug::log(Debug::Category::LIGHTS,
                 "LightManager: Maximum light count reached!");
      return INVALID_LIGHT_ID;
    }

    Debug::log(Debug::Category::LIGHTS, "LightManager: Adding light '",
               light.name, "'");

    LightID id = static_cast<LightID>(lights.size() + 1);
    lights.push_back(std::make_unique<Light>(light));

    if (light.castsShadows &&
        shadowSystem->getShadowMaps().size() < MAX_SHADOW_CASTERS) {
      uint32_t shadowMapIndex = shadowSystem->createShadowMap(
          static_cast<uint32_t>(lights.size() - 1));
      lights.back()->shadowMapIndex = shadowMapIndex;
      Debug::log(Debug::Category::LIGHTS,
                 "  - Created shadow map with index: ", shadowMapIndex);
    }

    updateLightBuffer();

    Debug::log(Debug::Category::LIGHTS,
               "LightManager: Successfully added light with ID: ", id);
    return id;
  }

  Light* getLight(LightID id) {
    if (id == INVALID_LIGHT_ID || id > lights.size()) {
      return nullptr;
    }
    return lights[id - 1].get();
  }

  void updateLight(LightID id, const Light& light) {
    if (id == INVALID_LIGHT_ID || id > lights.size()) {
      return;
    }
    *lights[id - 1] = light;
    updateLightBuffer();
  }

  void removeLight(LightID id) {
    if (id == INVALID_LIGHT_ID || id > lights.size()) {
      return;
    }
    lights.erase(lights.begin() + (id - 1));
    updateLightBuffer();
  }

  void updateLightBuffer() {
    LightBufferObject lbo{};
    lbo.numLights = static_cast<int>(lights.size());
    lbo.numShadowMaps = static_cast<int>(shadowSystem->getShadowMaps().size());

    for (size_t i = 0; i < lights.size(); i++) {
      lbo.lights[i].position = lights[i]->position;
      lbo.lights[i].type = static_cast<int>(lights[i]->type);
      lbo.lights[i].direction = lights[i]->direction;
      lbo.lights[i].intensity = lights[i]->intensity;
      lbo.lights[i].color = lights[i]->color;
      lbo.lights[i].constant = lights[i]->constant;
      lbo.lights[i].linear = lights[i]->linear;
      lbo.lights[i].quadratic = lights[i]->quadratic;
      lbo.lights[i].castsShadows = lights[i]->castsShadows ? 1 : 0;
      lbo.lights[i].shadowMapIndex =
          static_cast<int>(lights[i]->shadowMapIndex);

      if (lights[i]->shadowMapIndex != UINT32_MAX) {
        lbo.lights[i].lightSpaceMatrix =
            shadowSystem->getLightSpaceMatrix(lights[i]->shadowMapIndex);
      } else {
        lbo.lights[i].lightSpaceMatrix = glm::mat4(1.0f);
      }
    }

    memcpy(lightBufferMapped, &lbo, sizeof(LightBufferObject));
  }

  void updateLightSpaceMatrix(LightID id, const glm::mat4& matrix) {
    if (id == INVALID_LIGHT_ID || id > lights.size()) {
      return;
    }

    Light* light = lights[id - 1].get();
    if (light->shadowMapIndex != UINT32_MAX) {
      shadowSystem->updateLightSpaceMatrix(light->shadowMapIndex, matrix);
      updateLightBuffer();
    }
  }

  void cleanup() {
    Debug::log(Debug::Category::LIGHTS, "LightManager: Cleaning up");

    if (shadowSystem) {
      shadowSystem->cleanup();
      shadowSystem.reset();
    }

    if (lightBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(renderDevice->getDevice(), lightBuffer, nullptr);
      lightBuffer = VK_NULL_HANDLE;
    }
    if (lightBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(renderDevice->getDevice(), lightBufferMemory, nullptr);
      lightBufferMemory = VK_NULL_HANDLE;
    }

    lights.clear();

    Debug::log(Debug::Category::LIGHTS, "LightManager: Cleanup complete");
  }

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

  void updateAllShadowMatrices(const glm::vec3& sceneCenter,
                               float sceneRadius) {
    for (size_t i = 0; i < lights.size(); i++) {
      Light* light = lights[i].get();

      if (light->castsShadows && light->shadowMapIndex != UINT32_MAX) {
        const glm::mat4 lightSpaceMatrix =
            shadowSystem->calculateLightSpaceMatrix(*light, sceneCenter,
                                                    sceneRadius);

        shadowSystem->updateLightSpaceMatrix(light->shadowMapIndex,
                                             lightSpaceMatrix);
      }
    }

    updateLightBuffer();
  }
  void setShadowCastingEnabled(LightID id, bool enabled) {
    if (id == INVALID_LIGHT_ID || id > lights.size()) {
      return;
    }

    Light* light = lights[id - 1].get();
    const bool wasEnabled = light->castsShadows;
    light->castsShadows = enabled;

    if (enabled && !wasEnabled && light->shadowMapIndex == UINT32_MAX) {
      if (shadowSystem->getShadowMaps().size() < MAX_SHADOW_CASTERS) {
        uint32_t shadowMapIndex =
            shadowSystem->createShadowMap(static_cast<uint32_t>(id - 1));
        light->shadowMapIndex = shadowMapIndex;
        Debug::log(Debug::Category::LIGHTS,
                   "LightManager: Enabled shadows for light '", light->name,
                   "' with shadow map index: ", shadowMapIndex);
      }
    } else if (!enabled && wasEnabled) {
      light->shadowMapIndex = UINT32_MAX;
      Debug::log(Debug::Category::LIGHTS,
                 "LightManager: Disabled shadows for light '", light->name,
                 "'");
    }

    updateLightBuffer();
  }

  void debugPrintLightInfo() const {
    Debug::log(Debug::Category::LIGHTS, "=== Light System Debug Info ===");
    Debug::log(Debug::Category::LIGHTS, "Total lights: ", lights.size());

    for (size_t i = 0; i < lights.size(); i++) {
      const Light* light = lights[i].get();
      Debug::log(Debug::Category::LIGHTS, "Light ", i, ": ", light->name);
      Debug::log(Debug::Category::LIGHTS,
                 "  Type: ", (light->type == LightType::Sun ? "Sun" : "Point"));
      Debug::log(Debug::Category::LIGHTS, "  Position: (", light->position.x,
                 ", ", light->position.y, ", ", light->position.z, ")");
      Debug::log(Debug::Category::LIGHTS, "  Direction: (", light->direction.x,
                 ", ", light->direction.y, ", ", light->direction.z, ")");
      Debug::log(Debug::Category::LIGHTS, "  Intensity: ", light->intensity);
      Debug::log(Debug::Category::LIGHTS,
                 "  Casts Shadows: ", (light->castsShadows ? "Yes" : "No"));
      Debug::log(Debug::Category::LIGHTS,
                 "  Shadow Map Index: ", light->shadowMapIndex);
    }

    Debug::log(Debug::Category::LIGHTS,
               "Shadow maps: ", shadowSystem->getShadowMapCount());
  }

 private:
  std::vector<std::unique_ptr<Light>> lights;
  RenderDevice* renderDevice;

  void* lightBufferMapped;
  VkBuffer lightBuffer;
  VkDeviceMemory lightBufferMemory;

  std::unique_ptr<ShadowSystem> shadowSystem;

  void createLightBuffer() {
    Debug::log(Debug::Category::LIGHTS, "LightManager: Creating light buffer");

    const VkDeviceSize bufferSize = sizeof(LightBufferObject);

    renderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               lightBuffer, lightBufferMemory);

    vkMapMemory(renderDevice->getDevice(), lightBufferMemory, 0, bufferSize, 0,
                &lightBufferMapped);

    LightBufferObject lbo{};
    lbo.numLights = 0;
    lbo.numShadowMaps = 0;
    memcpy(lightBufferMapped, &lbo, sizeof(LightBufferObject));

    Debug::log(Debug::Category::LIGHTS, "LightManager: Light buffer created");
  }
};