#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <stdexcept>
#include <vector>

#include "../Rendering/RenderDevice.h"
#include "../Util/Debug.h"
#include "Light.h"

constexpr uint32_t SHADOW_MAP_SIZE = 16384;
constexpr uint32_t MAX_SHADOW_CASTERS = 4;

struct ShadowMapData {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView imageView;
  VkSampler sampler;
  glm::mat4 lightSpaceMatrix;
  uint32_t lightIndex;
};

class ShadowSystem {
 public:
  ShadowSystem(RenderDevice* device) : renderDevice(device) {
    Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Constructor called");
  }

  ~ShadowSystem() {
    Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Destructor called");
  }

  void init() {
    Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Initializing");
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();
    createDummyShadowMap();
    Debug::log(Debug::Category::SHADOWS,
               "ShadowSystem: Initialization complete");
  }

  uint32_t createShadowMap(uint32_t lightIndex) {
    Debug::log(Debug::Category::SHADOWS,
               "ShadowSystem: Creating shadow map for light index ",
               lightIndex);

    if (shadowMaps.size() >= MAX_SHADOW_CASTERS) {
      Debug::log(Debug::Category::SHADOWS,
                 "ShadowSystem: Maximum shadow casters reached!");
      return UINT32_MAX;
    }

    ShadowMapData shadowMap{};
    shadowMap.lightIndex = lightIndex;
    shadowMap.lightSpaceMatrix = glm::mat4(1.0f);

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
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(renderDevice->getDevice(), &imageInfo, nullptr,
                      &shadowMap.image) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create shadow map image!");
    }

    Debug::log(Debug::Category::SHADOWS, "  - Created shadow map image (",
               SHADOW_MAP_SIZE, "x", SHADOW_MAP_SIZE, ")");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(renderDevice->getDevice(), shadowMap.image,
                                 &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = renderDevice->findMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(renderDevice->getDevice(), &allocInfo, nullptr,
                         &shadowMap.memory) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate shadow map memory!");
    }

    vkBindImageMemory(renderDevice->getDevice(), shadowMap.image,
                      shadowMap.memory, 0);

    Debug::log(Debug::Category::SHADOWS, "  - Allocated shadow map memory");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = shadowMap.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(renderDevice->getDevice(), &viewInfo, nullptr,
                          &shadowMap.imageView) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create shadow map image view!");
    }

    Debug::log(Debug::Category::SHADOWS, "  - Created shadow map image view");

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(renderDevice->getDevice(), &samplerInfo, nullptr,
                        &shadowMap.sampler) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create shadow map sampler!");
    }

    Debug::log(Debug::Category::SHADOWS, "  - Created shadow map sampler");

    transitionImageLayout(shadowMap.image, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

    Debug::log(Debug::Category::SHADOWS,
               "  - Transitioned to read-only layout");

    shadowMaps.push_back(shadowMap);
    updateDescriptorSet();

    Debug::log(Debug::Category::SHADOWS,
               "ShadowSystem: Shadow map created successfully, total maps: ",
               shadowMaps.size());
    return static_cast<uint32_t>(shadowMaps.size() - 1);
  }

  void updateLightSpaceMatrix(uint32_t shadowMapIndex,
                              const glm::mat4& matrix) {
    if (shadowMapIndex < shadowMaps.size()) {
      shadowMaps[shadowMapIndex].lightSpaceMatrix = matrix;
    } else {
      Debug::log(Debug::Category::SHADOWS,
                 "ShadowSystem: WARNING - Invalid shadow map index ",
                 shadowMapIndex, " for light space matrix update");
    }
  }

  glm::mat4 getLightSpaceMatrix(uint32_t shadowMapIndex) const {
    if (shadowMapIndex < shadowMaps.size()) {
      return shadowMaps[shadowMapIndex].lightSpaceMatrix;
    }
    return glm::mat4(1.0f);
  }

  void cleanup() {
    Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Cleaning up ",
               shadowMaps.size(), " shadow maps");

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

    if (dummyShadowMap.sampler != VK_NULL_HANDLE) {
      vkDestroySampler(renderDevice->getDevice(), dummyShadowMap.sampler,
                       nullptr);
    }
    if (dummyShadowMap.imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(renderDevice->getDevice(), dummyShadowMap.imageView,
                         nullptr);
    }
    if (dummyShadowMap.image != VK_NULL_HANDLE) {
      vkDestroyImage(renderDevice->getDevice(), dummyShadowMap.image, nullptr);
    }
    if (dummyShadowMap.memory != VK_NULL_HANDLE) {
      vkFreeMemory(renderDevice->getDevice(), dummyShadowMap.memory, nullptr);
    }

    if (shadowDescriptorSet != VK_NULL_HANDLE) {
      shadowDescriptorSet = VK_NULL_HANDLE;
    }
    if (shadowDescriptorPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(renderDevice->getDevice(), shadowDescriptorPool,
                              nullptr);
      shadowDescriptorPool = VK_NULL_HANDLE;
    }
    if (shadowDescriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(renderDevice->getDevice(),
                                   shadowDescriptorSetLayout, nullptr);
      shadowDescriptorSetLayout = VK_NULL_HANDLE;
    }

    Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Cleanup complete");
  }

  const std::vector<ShadowMapData>& getShadowMaps() const { return shadowMaps; }
  uint32_t getShadowMapCount() const {
    return static_cast<uint32_t>(shadowMaps.size());
  }

  VkDescriptorSetLayout getShadowDescriptorSetLayout() const {
    return shadowDescriptorSetLayout;
  }
  VkDescriptorSet getShadowDescriptorSet() const { return shadowDescriptorSet; }

 private:
  RenderDevice* renderDevice;
  std::vector<ShadowMapData> shadowMaps;
  ShadowMapData dummyShadowMap{};

  VkDescriptorSetLayout shadowDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool shadowDescriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet shadowDescriptorSet = VK_NULL_HANDLE;

  void createDescriptorSetLayout();
  void createDescriptorPool();
  void createDescriptorSet();
  void createDummyShadowMap();
  void updateDescriptorSet();
  void transitionImageLayout(VkImage image, VkImageLayout oldLayout,
                             VkImageLayout newLayout);
};

inline void ShadowSystem::transitionImageLayout(VkImage image,
                                                VkImageLayout oldLayout,
                                                VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = renderDevice->beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::runtime_error("Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  renderDevice->endSingleTimeCommands(commandBuffer);
}

inline void ShadowSystem::createDescriptorSetLayout() {
  Debug::log(Debug::Category::SHADOWS,
             "ShadowSystem: Creating descriptor set layout");

  VkDescriptorSetLayoutBinding shadowMapBinding{};
  shadowMapBinding.binding = 0;
  shadowMapBinding.descriptorCount = MAX_SHADOW_CASTERS;
  shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &shadowMapBinding;

  if (vkCreateDescriptorSetLayout(renderDevice->getDevice(), &layoutInfo,
                                  nullptr,
                                  &shadowDescriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow descriptor set layout!");
  }

  Debug::log(Debug::Category::SHADOWS,
             "ShadowSystem: Descriptor set layout created");
}

inline void ShadowSystem::createDescriptorPool() {
  Debug::log(Debug::Category::SHADOWS,
             "ShadowSystem: Creating descriptor pool");

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = MAX_SHADOW_CASTERS;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 1;

  if (vkCreateDescriptorPool(renderDevice->getDevice(), &poolInfo, nullptr,
                             &shadowDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow descriptor pool!");
  }

  Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Descriptor pool created");
}

inline void ShadowSystem::createDescriptorSet() {
  Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Creating descriptor set");

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = shadowDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &shadowDescriptorSetLayout;

  if (vkAllocateDescriptorSets(renderDevice->getDevice(), &allocInfo,
                               &shadowDescriptorSet) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate shadow descriptor set!");
  }

  Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Descriptor set created");
}

inline void ShadowSystem::createDummyShadowMap() {
  Debug::log(Debug::Category::SHADOWS,
             "ShadowSystem: Creating dummy shadow map");

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
                    &dummyShadowMap.image) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create dummy shadow map image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(renderDevice->getDevice(), dummyShadowMap.image,
                               &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = renderDevice->findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(renderDevice->getDevice(), &allocInfo, nullptr,
                       &dummyShadowMap.memory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate dummy shadow map memory!");
  }

  vkBindImageMemory(renderDevice->getDevice(), dummyShadowMap.image,
                    dummyShadowMap.memory, 0);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = dummyShadowMap.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_D32_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderDevice->getDevice(), &viewInfo, nullptr,
                        &dummyShadowMap.imageView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create dummy shadow map image view!");
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

  if (vkCreateSampler(renderDevice->getDevice(), &samplerInfo, nullptr,
                      &dummyShadowMap.sampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create dummy shadow map sampler!");
  }

  transitionImageLayout(dummyShadowMap.image, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

  Debug::log(Debug::Category::SHADOWS,
             "ShadowSystem: Dummy shadow map created");
}

inline void ShadowSystem::updateDescriptorSet() {
  Debug::log(Debug::Category::SHADOWS,
             "ShadowSystem: Updating descriptor set with ", shadowMaps.size(),
             " shadow maps");

  std::vector<VkDescriptorImageInfo> imageInfos(MAX_SHADOW_CASTERS);

  for (size_t i = 0; i < MAX_SHADOW_CASTERS; i++) {
    if (i < shadowMaps.size()) {
      imageInfos[i].imageLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      imageInfos[i].imageView = shadowMaps[i].imageView;
      imageInfos[i].sampler = shadowMaps[i].sampler;
      Debug::log(Debug::Category::SHADOWS, "  - Binding shadow map ", i,
                 " to descriptor set");
    } else {
      imageInfos[i].imageLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      imageInfos[i].imageView = dummyShadowMap.imageView;
      imageInfos[i].sampler = dummyShadowMap.sampler;
      Debug::log(Debug::Category::SHADOWS,
                 "  - Binding dummy shadow map to slot ", i);
    }
  }

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = shadowDescriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = MAX_SHADOW_CASTERS;
  descriptorWrite.pImageInfo = imageInfos.data();

  vkUpdateDescriptorSets(renderDevice->getDevice(), 1, &descriptorWrite, 0,
                         nullptr);

  Debug::log(Debug::Category::SHADOWS, "ShadowSystem: Descriptor set updated");
}