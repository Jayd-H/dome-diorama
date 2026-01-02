#include "LightManager.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>

#include "Util/Debug.h"

LightManager::LightManager(RenderDevice* device)
    : renderDevice(device),
      lightBuffer(VK_NULL_HANDLE),
      lightBufferMemory(VK_NULL_HANDLE),
      lightBufferMapped(nullptr),
      shadowSystem(nullptr) {
  Debug::log(Debug::Category::LIGHTS, "LightManager: Constructor called");
}

LightManager::~LightManager() {
  Debug::log(Debug::Category::LIGHTS, "LightManager: Destructor called");
  cleanup();
}

void LightManager::init() {
  Debug::log(Debug::Category::LIGHTS, "LightManager: Initializing");
  createLightBuffer();
  shadowSystem = std::make_unique<ShadowSystem>(renderDevice);
  shadowSystem->init();
  Debug::log(Debug::Category::LIGHTS, "LightManager: Initialization complete");
}

LightID LightManager::addLight(const Light& light) {
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
    uint32_t shadowMapIndex =
        shadowSystem->createShadowMap(static_cast<uint32_t>(lights.size() - 1));
    lights.back()->shadowMapIndex = shadowMapIndex;
    Debug::log(Debug::Category::LIGHTS,
               "  - Created shadow map with index: ", shadowMapIndex);
  }

  updateLightBuffer();

  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Successfully added light with ID: ", id);
  return id;
}

Light* LightManager::getLight(LightID id) {
  if (id == INVALID_LIGHT_ID || id > lights.size()) {
    return nullptr;
  }
  return lights[id - 1].get();
}

void LightManager::updateLight(LightID id, const Light& light) {
  if (id == INVALID_LIGHT_ID || id > lights.size()) {
    return;
  }
  *lights[id - 1] = light;
  updateLightBuffer();
}

void LightManager::removeLight(LightID id) {
  if (id == INVALID_LIGHT_ID || id > lights.size()) {
    return;
  }
  lights.erase(lights.begin() + (id - 1));
  updateLightBuffer();
}

void LightManager::updateLightBuffer() {
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
    lbo.lights[i].cutOff = lights[i]->cutOff;
    lbo.lights[i].outerCutOff = lights[i]->outerCutOff;
    lbo.lights[i].castsShadows = lights[i]->castsShadows ? 1 : 0;
    lbo.lights[i].shadowMapIndex = static_cast<int>(lights[i]->shadowMapIndex);

    if (lights[i]->shadowMapIndex != UINT32_MAX) {
      lbo.lights[i].lightSpaceMatrix =
          shadowSystem->getLightSpaceMatrix(lights[i]->shadowMapIndex);
    } else {
      lbo.lights[i].lightSpaceMatrix = glm::mat4(1.0f);
    }
  }

  memcpy(lightBufferMapped, &lbo, sizeof(LightBufferObject));
}

void LightManager::updateLightSpaceMatrix(LightID id, const glm::mat4& matrix) {
  if (id == INVALID_LIGHT_ID || id > lights.size()) {
    return;
  }

  Light* light = lights[id - 1].get();
  if (light->shadowMapIndex != UINT32_MAX) {
    shadowSystem->updateLightSpaceMatrix(light->shadowMapIndex, matrix);
    updateLightBuffer();
  }
}

void LightManager::cleanup() {
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

void LightManager::createLightBuffer() {
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