#include "LightManager.h"

#include "Debug.h"

LightManager::LightManager(RenderDevice* renderDevice)
    : renderDevice(renderDevice),
      lightBuffer(VK_NULL_HANDLE),
      lightBufferMemory(VK_NULL_HANDLE),
      lightBufferMapped(nullptr) {
  Debug::log(Debug::Category::RENDERING, "LightManager: Constructor called");
}

LightManager::~LightManager() {
  Debug::log(Debug::Category::RENDERING, "LightManager: Destructor called");
  cleanup();
}

void LightManager::init() {
  Debug::log(Debug::Category::RENDERING, "LightManager: Initializing");
  createLightBuffer();
  Debug::log(Debug::Category::RENDERING,
             "LightManager: Initialization complete");
}

LightID LightManager::addLight(const Light& light) {
  if (lights.size() >= MAX_LIGHTS) {
    Debug::log(Debug::Category::RENDERING,
               "LightManager: Maximum light count reached!");
    return INVALID_LIGHT_ID;
  }

  Debug::log(Debug::Category::RENDERING, "LightManager: Adding light '",
             light.name, "'");

  LightID id = static_cast<LightID>(lights.size() + 1);
  lights.push_back(std::make_unique<Light>(light));

  updateLightBuffer();

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Successfully added light with ID: ", id);

  return id;
}

Light* LightManager::getLight(LightID id) {
  if (id == INVALID_LIGHT_ID || id > lights.size()) {
    Debug::log(Debug::Category::RENDERING,
               "LightManager: Invalid light ID requested: ", id);
    return nullptr;
  }
  return lights[id - 1].get();
}

void LightManager::updateLight(LightID id, const Light& light) {
  if (id == INVALID_LIGHT_ID || id > lights.size()) {
    Debug::log(Debug::Category::RENDERING,
               "LightManager: Cannot update invalid light ID: ", id);
    return;
  }

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Updating light ID: ", id);

  *lights[id - 1] = light;
  updateLightBuffer();
}

void LightManager::removeLight(LightID id) {
  if (id == INVALID_LIGHT_ID || id > lights.size()) {
    Debug::log(Debug::Category::RENDERING,
               "LightManager: Cannot remove invalid light ID: ", id);
    return;
  }

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Removing light ID: ", id);

  lights.erase(lights.begin() + (id - 1));
  updateLightBuffer();
}

void LightManager::updateLightBuffer() {
  LightBufferObject lbo{};
  lbo.numLights = static_cast<int>(lights.size());

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
  }

  memcpy(lightBufferMapped, &lbo, sizeof(LightBufferObject));
}

void LightManager::cleanup() {
  Debug::log(Debug::Category::RENDERING, "LightManager: Cleaning up");

  if (lightBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(renderDevice->getDevice(), lightBuffer, nullptr);
  }
  if (lightBufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(renderDevice->getDevice(), lightBufferMemory, nullptr);
  }

  lights.clear();

  Debug::log(Debug::Category::RENDERING, "LightManager: Cleanup complete");
}

void LightManager::createLightBuffer() {
  Debug::log(Debug::Category::RENDERING, "LightManager: Creating light buffer");

  VkDeviceSize bufferSize = sizeof(LightBufferObject);

  renderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             lightBuffer, lightBufferMemory);

  vkMapMemory(renderDevice->getDevice(), lightBufferMemory, 0, bufferSize, 0,
              &lightBufferMapped);

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Light buffer created successfully");
}