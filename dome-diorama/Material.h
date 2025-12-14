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
  float textureScale = 1.0f;
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
  MaterialBuilder() { material.properties = MaterialProperties{}; }

  MaterialBuilder& name(const std::string& n) {
    material.name = n;
    return *this;
  }

  MaterialBuilder& albedoMap(TextureID id) {
    material.albedoMap = id;
    return *this;
  }

  MaterialBuilder& albedoMap(const std::string& filepath) {
    hasAlbedoTexture = true;
    albedoFilepath = filepath;
    return *this;
  }

  MaterialBuilder& albedoColor(const glm::vec3& color) {
    material.properties.albedoColor = glm::vec4(color, 1.0f);
    return *this;
  }

  MaterialBuilder& albedoColor(float r, float g, float b) {
    material.properties.albedoColor = glm::vec4(r, g, b, 1.0f);
    return *this;
  }

  MaterialBuilder& normalMap(TextureID id) {
    material.normalMap = id;
    return *this;
  }

  MaterialBuilder& normalMap(const std::string& filepath) {
    hasNormalTexture = true;
    normalFilepath = filepath;
    return *this;
  }

  MaterialBuilder& roughnessMap(TextureID id) {
    material.roughnessMap = id;
    return *this;
  }

  MaterialBuilder& roughnessMap(const std::string& filepath) {
    hasRoughnessTexture = true;
    roughnessFilepath = filepath;
    return *this;
  }

  MaterialBuilder& roughness(float value) {
    material.properties.roughness = value;
    return *this;
  }

  MaterialBuilder& metallicMap(TextureID id) {
    material.metallicMap = id;
    return *this;
  }

  MaterialBuilder& metallicMap(const std::string& filepath) {
    hasMetallicTexture = true;
    metallicFilepath = filepath;
    return *this;
  }

  MaterialBuilder& metallic(float value) {
    material.properties.metallic = value;
    return *this;
  }

  MaterialBuilder& emissiveMap(TextureID id) {
    material.emissiveMap = id;
    return *this;
  }

  MaterialBuilder& emissiveMap(const std::string& filepath) {
    hasEmissiveTexture = true;
    emissiveFilepath = filepath;
    return *this;
  }

  MaterialBuilder& emissiveIntensity(float value) {
    material.properties.emissiveIntensity = value;
    return *this;
  }

  MaterialBuilder& heightMap(TextureID id) {
    material.heightMap = id;
    return *this;
  }

  MaterialBuilder& heightMap(const std::string& filepath) {
    hasHeightTexture = true;
    heightFilepath = filepath;
    return *this;
  }

  MaterialBuilder& heightScale(float value) {
    material.properties.heightScale = value;
    return *this;
  }

  MaterialBuilder& aoMap(TextureID id) {
    material.aoMap = id;
    return *this;
  }

  MaterialBuilder& aoMap(const std::string& filepath) {
    hasAOTexture = true;
    aoFilepath = filepath;
    return *this;
  }

  MaterialBuilder& transparent(bool enabled = true) {
    material.isTransparent = enabled;
    return *this;
  }

  MaterialBuilder& opacity(float value) {
    material.properties.opacity = value;
    return *this;
  }

  MaterialBuilder& indexOfRefraction(float ior) {
    material.properties.indexOfRefraction = ior;
    return *this;
  }

  MaterialBuilder& doubleSided(bool enabled = true) {
    material.doubleSided = enabled;
    return *this;
  }

  MaterialBuilder& textureScale(float value) {
    material.properties.textureScale = value;
    return *this;
  }

  Material* build() { return new Material(material); }

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

 private:
  Material material;
};