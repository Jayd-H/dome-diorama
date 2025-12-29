#include "LightManager.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>

#include "Debug.h"

LightManager::LightManager(RenderDevice* device)
    : renderDevice(device),
      lightBuffer(VK_NULL_HANDLE),
      lightBufferMemory(VK_NULL_HANDLE),
      lightBufferMapped(nullptr),
      shadowDescriptorSetLayout(VK_NULL_HANDLE),
      shadowDescriptorPool(VK_NULL_HANDLE),
      shadowDescriptorSet(VK_NULL_HANDLE) {
  Debug::log(Debug::Category::RENDERING, "LightManager: Constructor called");
}

LightManager::~LightManager() {
  try {
    Debug::log(Debug::Category::RENDERING, "LightManager: Destructor called");
    cleanup();
  } catch (...) {
  }
}

void LightManager::init() {
  Debug::log(Debug::Category::RENDERING, "LightManager: Initializing");
  createLightBuffer();
  createShadowDescriptorSetLayout();
  createShadowDescriptorPool();
  createShadowDescriptorSet();
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

  if (light.castsShadows) {
    createShadowMapForLight(lights.back().get());
    updateShadowDescriptorSet();
  }

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

  const Light* light = lights[id - 1].get();
  if (light->castsShadows) {
    cleanupShadowResources();
  }

  lights.erase(lights.begin() + (id - 1));
  updateLightBuffer();
}

void LightManager::updateLightBuffer() {
  LightBufferObject lbo{};
  lbo.numLights = static_cast<int>(lights.size());

  int shadowMapIndex = 0;
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
    lbo.lights[i].shadowMapIndex =
        lights[i]->castsShadows ? shadowMapIndex++ : -1;
    lbo.lights[i].lightSpaceMatrix = lights[i]->lightSpaceMatrix;
  }

  memcpy(lightBufferMapped, &lbo, sizeof(LightBufferObject));
}

void LightManager::updateLightSpaceMatrix(LightID id, const glm::mat4& matrix) {
  if (id == INVALID_LIGHT_ID || id > lights.size()) {
    return;
  }

  lights[id - 1]->lightSpaceMatrix = matrix;
  updateLightBuffer();
}

void LightManager::cleanup() {
  Debug::log(Debug::Category::RENDERING, "LightManager: Cleaning up");

  cleanupShadowResources();

  if (shadowDescriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(renderDevice->getDevice(),
                                 shadowDescriptorSetLayout, nullptr);
  }

  if (shadowDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(renderDevice->getDevice(), shadowDescriptorPool,
                            nullptr);
  }

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

  const VkDeviceSize bufferSize = sizeof(LightBufferObject);

  renderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             lightBuffer, lightBufferMemory);

  vkMapMemory(renderDevice->getDevice(), lightBufferMemory, 0, bufferSize, 0,
              &lightBufferMapped);

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Light buffer created successfully");
}

void LightManager::createShadowMapForLight(Light* light) {
  Debug::log(Debug::Category::RENDERING,
             "LightManager: Creating shadow map for light '", light->name, "'");

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = SHADOW_MAP_SIZE;
  imageInfo.extent.height = SHADOW_MAP_SIZE;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_D32_SFLOAT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(renderDevice->getDevice(), &imageInfo, nullptr,
                    &light->shadowMap) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow map image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(renderDevice->getDevice(), light->shadowMap,
                               &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = renderDevice->findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(renderDevice->getDevice(), &allocInfo, nullptr,
                       &light->shadowMapMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate shadow map memory!");
  }

  vkBindImageMemory(renderDevice->getDevice(), light->shadowMap,
                    light->shadowMapMemory, 0);

  const VkCommandBuffer commandBuffer = renderDevice->beginSingleTimeCommands();

  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
  barrier.srcAccessMask = 0;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
  barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  barrier.image = light->shadowMap;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers = &barrier;

  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  renderDevice->endSingleTimeCommands(commandBuffer);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = light->shadowMap;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_D32_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderDevice->getDevice(), &viewInfo, nullptr,
                        &light->shadowMapView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow map image view!");
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_TRUE;
  samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(renderDevice->getDevice(), &samplerInfo, nullptr,
                      &light->shadowMapSampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow map sampler!");
  }

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Shadow map created successfully");
}

void LightManager::createShadowDescriptorSetLayout() {
  Debug::log(Debug::Category::RENDERING,
             "LightManager: Creating shadow descriptor set layout");

  std::array<VkDescriptorSetLayoutBinding, MAX_LIGHTS> bindings = {};

  for (uint32_t i = 0; i < MAX_LIGHTS; i++) {
    bindings[i].binding = i;
    bindings[i].descriptorCount = 1;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(renderDevice->getDevice(), &layoutInfo,
                                  nullptr,
                                  &shadowDescriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow descriptor set layout!");
  }

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Shadow descriptor set layout created");
}

void LightManager::createShadowDescriptorPool() {
  Debug::log(Debug::Category::RENDERING,
             "LightManager: Creating shadow descriptor pool");

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = MAX_LIGHTS;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 1;

  if (vkCreateDescriptorPool(renderDevice->getDevice(), &poolInfo, nullptr,
                             &shadowDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow descriptor pool!");
  }

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Shadow descriptor pool created");
}

void LightManager::createShadowDescriptorSet() {
  Debug::log(Debug::Category::RENDERING,
             "LightManager: Creating shadow descriptor set");

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = shadowDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &shadowDescriptorSetLayout;

  if (vkAllocateDescriptorSets(renderDevice->getDevice(), &allocInfo,
                               &shadowDescriptorSet) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate shadow descriptor set!");
  }

  updateShadowDescriptorSet();

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Shadow descriptor set created");
}

void LightManager::updateShadowDescriptorSet() {
  Debug::log(Debug::Category::RENDERING,
             "LightManager: Updating shadow descriptor set");

  std::array<VkDescriptorImageInfo, MAX_LIGHTS> imageInfos = {};
  std::array<VkWriteDescriptorSet, MAX_LIGHTS> descriptorWrites = {};

  VkImageView defaultView = VK_NULL_HANDLE;
  VkSampler defaultSampler = VK_NULL_HANDLE;

  int shadowMapIndex = 0;
  for (size_t i = 0; i < lights.size() && shadowMapIndex < MAX_LIGHTS; i++) {
    if (lights[i]->castsShadows) {
      imageInfos[shadowMapIndex].imageLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      imageInfos[shadowMapIndex].imageView = lights[i]->shadowMapView;
      imageInfos[shadowMapIndex].sampler = lights[i]->shadowMapSampler;

      if (defaultView == VK_NULL_HANDLE) {
        defaultView = lights[i]->shadowMapView;
        defaultSampler = lights[i]->shadowMapSampler;
      }

      shadowMapIndex++;
    }
  }

  for (uint32_t i = 0; i < MAX_LIGHTS; i++) {
    if (i >= static_cast<uint32_t>(shadowMapIndex)) {
      if (defaultView != VK_NULL_HANDLE) {
        imageInfos[i].imageLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        imageInfos[i].imageView = defaultView;
        imageInfos[i].sampler = defaultSampler;
      }
    }

    descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[i].dstSet = shadowDescriptorSet;
    descriptorWrites[i].dstBinding = i;
    descriptorWrites[i].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[i].descriptorCount = 1;
    descriptorWrites[i].pImageInfo = &imageInfos[i];
  }

  if (defaultView != VK_NULL_HANDLE) {
    vkUpdateDescriptorSets(renderDevice->getDevice(), MAX_LIGHTS,
                           descriptorWrites.data(), 0, nullptr);
  }

  Debug::log(Debug::Category::RENDERING,
             "LightManager: Shadow descriptor set updated");
}

void LightManager::cleanupShadowResources() {
  for (auto& light : lights) {
    if (light->castsShadows) {
      if (light->shadowMapSampler != VK_NULL_HANDLE) {
        vkDestroySampler(renderDevice->getDevice(), light->shadowMapSampler,
                         nullptr);
        light->shadowMapSampler = VK_NULL_HANDLE;
      }
      if (light->shadowMapView != VK_NULL_HANDLE) {
        vkDestroyImageView(renderDevice->getDevice(), light->shadowMapView,
                           nullptr);
        light->shadowMapView = VK_NULL_HANDLE;
      }
      if (light->shadowMap != VK_NULL_HANDLE) {
        vkDestroyImage(renderDevice->getDevice(), light->shadowMap, nullptr);
        light->shadowMap = VK_NULL_HANDLE;
      }
      if (light->shadowMapMemory != VK_NULL_HANDLE) {
        vkFreeMemory(renderDevice->getDevice(), light->shadowMapMemory,
                     nullptr);
        light->shadowMapMemory = VK_NULL_HANDLE;
      }
    }
  }
}