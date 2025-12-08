#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

class RenderDevice;

class PostProcessing {
 public:
  PostProcessing(RenderDevice* renderDevice, VkDevice device,
                 VkFormat swapchainFormat);
  ~PostProcessing();

  void init(VkDescriptorPool descriptorPool, uint32_t width, uint32_t height);
  void cleanup();

  void resize(uint32_t width, uint32_t height, VkDescriptorPool descriptorPool);

  void beginOffscreenPass(VkCommandBuffer commandBuffer,
                          VkImageView depthImageView, VkExtent2D extent);
  void endOffscreenPass(VkCommandBuffer commandBuffer);

  void render(VkCommandBuffer commandBuffer, VkImageView targetImageView,
              VkExtent2D extent, uint32_t frameIndex);

  VkImageView getOffscreenImageView() const { return offscreenImageView; }

 private:
  RenderDevice* renderDevice;
  VkDevice device;
  VkFormat swapchainFormat;

  VkImage offscreenImage = VK_NULL_HANDLE;
  VkDeviceMemory offscreenImageMemory = VK_NULL_HANDLE;
  VkImageView offscreenImageView = VK_NULL_HANDLE;
  VkSampler offscreenSampler = VK_NULL_HANDLE;

  VkImage depthImage = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
  VkImageView depthImageView = VK_NULL_HANDLE;
  VkFormat depthFormat;

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  uint32_t width = 0;
  uint32_t height = 0;

  void createOffscreenResources();
  void createDepthResources();
  void createDescriptorSetLayout();
  void createPipeline();
  void createDescriptorSets(VkDescriptorPool descriptorPool);
  void updateDescriptorSets();

  void cleanupOffscreenResources();
  void cleanupDepthResources();

  VkShaderModule createShaderModule(const std::vector<char>& code);
  static std::vector<char> readFile(const std::string& filename);
};