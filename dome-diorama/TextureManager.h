#pragma once
#include <unordered_map>
#include <vector>

#include "Texture.h"

class TextureManager {
 public:
  TextureManager(VkDevice device, VkPhysicalDevice physicalDevice,
                 VkCommandPool commandPool, VkQueue graphicsQueue);
  ~TextureManager();
  TextureID load(const TextureCreateInfo& createInfo);
  TextureID load(const std::string& filepath,
                 TextureType type = TextureType::sRGB);
  VkImageView getImageView(TextureID id) const;
  VkSampler getSampler(TextureID id) const;
  TextureID getDefaultWhite() const { return defaultWhiteTexture; }
  TextureID getDefaultNormal() const { return defaultNormalTexture; }
  TextureID getDefaultBlack() const { return defaultBlackTexture; }

  void recreateSamplers(VkFilter magFilter, VkFilter minFilter);

  void cleanup();

 private:
  struct TextureData {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    VkFormat format = VK_FORMAT_UNDEFINED;
  };

  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkCommandPool commandPool;
  VkQueue graphicsQueue;

  std::unordered_map<std::string, TextureID> filepathToID;
  std::vector<TextureData> textures;

  TextureID defaultWhiteTexture;
  TextureID defaultNormalTexture;
  TextureID defaultBlackTexture;

  TextureID createTexture(const TextureCreateInfo& createInfo);
  TextureID createDefaultTexture(const unsigned char* pixelData, uint32_t width,
                                 uint32_t height, TextureType type);

  void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                   VkFormat format, VkImageTiling tiling,
                   VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                   VkImage& image, VkDeviceMemory& imageMemory);

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t mipLevels);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);

  void generateMipmaps(VkImage image, VkFormat format, uint32_t width,
                       uint32_t height, uint32_t mipLevels);

  VkSampler createSampler(TextureFilter filter, TextureWrap wrap,
                          uint32_t mipLevels) const;

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) const;

  VkCommandBuffer beginSingleTimeCommands() const;
  void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
};