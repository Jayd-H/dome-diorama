#pragma once
#pragma once
#include <vulkan/vulkan.h>

class RenderDevice {
 public:
  RenderDevice(VkDevice device, VkPhysicalDevice physicalDevice,
               VkCommandPool commandPool, VkQueue graphicsQueue);

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory);

  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  VkDevice getDevice() const { return device; }
  VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
  VkCommandPool getCommandPool() const { return commandPool; }
  VkQueue getGraphicsQueue() const { return graphicsQueue; }

 private:
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkCommandPool commandPool;
  VkQueue graphicsQueue;
};