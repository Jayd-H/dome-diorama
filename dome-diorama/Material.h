#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <string>

#include "Texture.h"

using MaterialID = uint32_t;
constexpr MaterialID INVALID_MATERIAL_ID = 0;

struct MaterialProperties {
  glm::vec4 albedoColor = glm::vec4(1.0f);

  float roughness = 0.5f;
  float metallic = 0.0f;
  float emissiveIntensity = 0.0f;
  float opacity = 1.0f;

  float indexOfRefraction = 1.5f;
  float heightScale = 0.05f;
  float padding1 = 0.0f;
  float padding2 = 0.0f;
};

class Material {
 public:
  TextureID albedoMap = INVALID_TEXTURE_ID;
  TextureID normalMap = INVALID_TEXTURE_ID;
  TextureID roughnessMap = INVALID_TEXTURE_ID;
  TextureID metallicMap = INVALID_TEXTURE_ID;
  TextureID emissiveMap = INVALID_TEXTURE_ID;
  TextureID heightMap = INVALID_TEXTURE_ID;
  TextureID aoMap = INVALID_TEXTURE_ID;

  MaterialProperties properties;

  VkBuffer propertiesBuffer = VK_NULL_HANDLE;
  VkDeviceMemory propertiesBufferMemory = VK_NULL_HANDLE;

  bool isTransparent = false;
  bool doubleSided = false;

  std::string name = "Unnamed Material";

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

class MaterialBuilder {
 public:
  MaterialBuilder();

  MaterialBuilder& name(const std::string& name);

  MaterialBuilder& albedoMap(TextureID id);
  MaterialBuilder& albedoMap(const std::string& filepath);
  MaterialBuilder& albedoColor(const glm::vec3& color);
  MaterialBuilder& albedoColor(float r, float g, float b);

  MaterialBuilder& normalMap(TextureID id);
  MaterialBuilder& normalMap(const std::string& filepath);

  MaterialBuilder& roughnessMap(TextureID id);
  MaterialBuilder& roughnessMap(const std::string& filepath);
  MaterialBuilder& roughness(float value);

  MaterialBuilder& metallicMap(TextureID id);
  MaterialBuilder& metallicMap(const std::string& filepath);
  MaterialBuilder& metallic(float value);

  MaterialBuilder& emissiveMap(TextureID id);
  MaterialBuilder& emissiveMap(const std::string& filepath);
  MaterialBuilder& emissiveIntensity(float value);

  MaterialBuilder& heightMap(TextureID id);
  MaterialBuilder& heightMap(const std::string& filepath);
  MaterialBuilder& heightScale(float value);

  MaterialBuilder& aoMap(TextureID id);
  MaterialBuilder& aoMap(const std::string& filepath);

  MaterialBuilder& transparent(bool enabled = true);
  MaterialBuilder& opacity(float value);
  MaterialBuilder& indexOfRefraction(float ior);

  MaterialBuilder& doubleSided(bool enabled = true);

  Material* build();

 private:
  Material material;

  bool hasAlbedoTexture = false;
  std::string albedoFilepath;

  bool hasNormalTexture = false;
  std::string normalFilepath;

  bool hasRoughnessTexture = false;
  std::string roughnessFilepath;

  bool hasMetallicTexture = false;
  std::string metallicFilepath;

  bool hasEmissiveTexture = false;
  std::string emissiveFilepath;

  bool hasHeightTexture = false;
  std::string heightFilepath;

  bool hasAOTexture = false;
  std::string aoFilepath;
};