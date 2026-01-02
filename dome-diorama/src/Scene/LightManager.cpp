#include "LightManager.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>

#include "Util/Debug.h"

LightManager::LightManager(RenderDevice* device)
    : renderDevice(device),
      lightBuffer(VK_NULL_HANDLE),
      lightBufferMemory(VK_NULL_HANDLE),
      lightBufferMapped(nullptr),
      shadowDescriptorSetLayout(VK_NULL_HANDLE),
      shadowDescriptorPool(VK_NULL_HANDLE),
      shadowDescriptorSet(VK_NULL_HANDLE),
      dummyShadowMap(VK_NULL_HANDLE),
      dummyShadowMapMemory(VK_NULL_HANDLE),
      dummyShadowMapView(VK_NULL_HANDLE),
      dummyShadowMapSampler(VK_NULL_HANDLE) {
  Debug::log(Debug::Category::LIGHTS, "LightManager: Constructor called");
}

LightManager::~LightManager() {
  Debug::log(Debug::Category::LIGHTS, "LightManager: Destructor called");
  cleanup();
}

void LightManager::init() {
  Debug::log(Debug::Category::LIGHTS, "LightManager: Initializing");
  createLightBuffer();
  createDummyShadowMap();
  createShadowDescriptorSetLayout();
  createShadowDescriptorPool();
  createShadowDescriptorSet();
  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Initialization complete");
}

void LightManager::createDummyShadowMap() {
  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Creating dummy shadow map");

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = 1;
  imageInfo.extent.height = 1;
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
                    &dummyShadowMap) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create dummy shadow map image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(renderDevice->getDevice(), dummyShadowMap,
                               &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = renderDevice->findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(renderDevice->getDevice(), &allocInfo, nullptr,
                       &dummyShadowMapMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate dummy shadow map memory!");
  }

  vkBindImageMemory(renderDevice->getDevice(), dummyShadowMap,
                    dummyShadowMapMemory, 0);

  const VkCommandBuffer commandBuffer = renderDevice->beginSingleTimeCommands();

  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
  barrier.srcAccessMask = 0;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
  barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  barrier.image = dummyShadowMap;
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
  viewInfo.image = dummyShadowMap;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_D32_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderDevice->getDevice(), &viewInfo, nullptr,
                        &dummyShadowMapView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create dummy shadow map image view!");
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_TRUE;
  samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(renderDevice->getDevice(), &samplerInfo, nullptr,
                      &dummyShadowMapSampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create dummy shadow map sampler!");
  }

  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Dummy shadow map created");
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

  if (light.castsShadows && shadowMaps.size() < MAX_SHADOW_CASTING_LIGHTS) {
    createShadowMapForLight(lights.back().get(), id);
    updateShadowDescriptorSet();
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
  lbo.numShadowMaps = static_cast<int>(shadowMaps.size());

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
    lbo.lights[i].lightSpaceMatrix = lights[i]->lightSpaceMatrix;

    lbo.lights[i].shadowMapIndex = -1;
    if (lights[i]->castsShadows) {
      for (size_t j = 0; j < shadowMaps.size(); j++) {
        if (shadowMaps[j].lightId == static_cast<LightID>(i + 1)) {
          lbo.lights[i].shadowMapIndex = static_cast<int>(j);
          break;
        }
      }
    }
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
  Debug::log(Debug::Category::LIGHTS, "LightManager: Cleaning up");

  cleanupShadowResources();

  if (dummyShadowMapSampler != VK_NULL_HANDLE) {
    vkDestroySampler(renderDevice->getDevice(), dummyShadowMapSampler, nullptr);
    dummyShadowMapSampler = VK_NULL_HANDLE;
  }
  if (dummyShadowMapView != VK_NULL_HANDLE) {
    vkDestroyImageView(renderDevice->getDevice(), dummyShadowMapView, nullptr);
    dummyShadowMapView = VK_NULL_HANDLE;
  }
  if (dummyShadowMap != VK_NULL_HANDLE) {
    vkDestroyImage(renderDevice->getDevice(), dummyShadowMap, nullptr);
    dummyShadowMap = VK_NULL_HANDLE;
  }
  if (dummyShadowMapMemory != VK_NULL_HANDLE) {
    vkFreeMemory(renderDevice->getDevice(), dummyShadowMapMemory, nullptr);
    dummyShadowMapMemory = VK_NULL_HANDLE;
  }

  if (shadowDescriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(renderDevice->getDevice(),
                                 shadowDescriptorSetLayout, nullptr);
    shadowDescriptorSetLayout = VK_NULL_HANDLE;
  }

  if (shadowDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(renderDevice->getDevice(), shadowDescriptorPool,
                            nullptr);
    shadowDescriptorPool = VK_NULL_HANDLE;
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

void LightManager::createShadowMapForLight(Light* light, LightID lightId) {
  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Creating shadow map for light '", light->name, "'");

  ShadowMapInfo info{};
  info.light = light;
  info.lightId = lightId;

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
                    &info.image) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow map image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(renderDevice->getDevice(), info.image,
                               &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = renderDevice->findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(renderDevice->getDevice(), &allocInfo, nullptr,
                       &info.memory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate shadow map memory!");
  }

  vkBindImageMemory(renderDevice->getDevice(), info.image, info.memory, 0);

  const VkCommandBuffer commandBuffer = renderDevice->beginSingleTimeCommands();

  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
  barrier.srcAccessMask = 0;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
  barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  barrier.image = info.image;
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
  viewInfo.image = info.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_D32_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderDevice->getDevice(), &viewInfo, nullptr,
                        &info.imageView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow map image view!");
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_TRUE;
  samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(renderDevice->getDevice(), &samplerInfo, nullptr,
                      &info.sampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow map sampler!");
  }

  shadowMaps.push_back(info);

  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Shadow map created (index: ", shadowMaps.size() - 1,
             ")");
}

void LightManager::createShadowDescriptorSetLayout() {
  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Creating shadow descriptor set layout");

  VkDescriptorSetLayoutBinding binding{};
  binding.binding = 0;
  binding.descriptorCount = MAX_SHADOW_CASTING_LIGHTS;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &binding;

  if (vkCreateDescriptorSetLayout(renderDevice->getDevice(), &layoutInfo,
                                  nullptr,
                                  &shadowDescriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow descriptor set layout!");
  }

  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Shadow descriptor set layout created");
}

void LightManager::createShadowDescriptorPool() {
  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Creating shadow descriptor pool");

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = MAX_SHADOW_CASTING_LIGHTS;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 1;

  if (vkCreateDescriptorPool(renderDevice->getDevice(), &poolInfo, nullptr,
                             &shadowDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow descriptor pool!");
  }

  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Shadow descriptor pool created");
}

void LightManager::createShadowDescriptorSet() {
  Debug::log(Debug::Category::LIGHTS,
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

  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Shadow descriptor set created");
}

void LightManager::updateShadowDescriptorSet() {
  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Updating shadow descriptor set");

  std::array<VkDescriptorImageInfo, MAX_SHADOW_CASTING_LIGHTS> imageInfos{};

  for (uint32_t i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++) {
    if (i < shadowMaps.size()) {
      imageInfos[i].imageLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      imageInfos[i].imageView = shadowMaps[i].imageView;
      imageInfos[i].sampler = shadowMaps[i].sampler;
    } else {
      imageInfos[i].imageLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      imageInfos[i].imageView = dummyShadowMapView;
      imageInfos[i].sampler = dummyShadowMapSampler;
    }
  }

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = shadowDescriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = MAX_SHADOW_CASTING_LIGHTS;
  descriptorWrite.pImageInfo = imageInfos.data();

  vkUpdateDescriptorSets(renderDevice->getDevice(), 1, &descriptorWrite, 0,
                         nullptr);

  Debug::log(Debug::Category::LIGHTS,
             "LightManager: Shadow descriptor set updated with ",
             shadowMaps.size(), " shadow maps");
}

void LightManager::cleanupShadowResources() {
  for (auto& shadowMap : shadowMaps) {
    if (shadowMap.sampler != VK_NULL_HANDLE) {
      vkDestroySampler(renderDevice->getDevice(), shadowMap.sampler, nullptr);
    }
    if (shadowMap.imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(renderDevice->getDevice(), shadowMap.imageView,
                         nullptr);
    }
    if (shadowMap.image != VK_NULL_HANDLE) {
      vkDestroyImage(renderDevice->getDevice(), shadowMap.image, nullptr);
    }
    if (shadowMap.memory != VK_NULL_HANDLE) {
      vkFreeMemory(renderDevice->getDevice(), shadowMap.memory, nullptr);
    }
  }
  shadowMaps.clear();
}