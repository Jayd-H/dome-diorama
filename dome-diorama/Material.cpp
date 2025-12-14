#include "Material.h"

MaterialBuilder::MaterialBuilder() {
  material.properties = MaterialProperties{};
}

MaterialBuilder& MaterialBuilder::name(const std::string& name) {
  material.name = name;
  return *this;
}

MaterialBuilder& MaterialBuilder::albedoMap(TextureID id) {
  material.albedoMap = id;
  return *this;
}

MaterialBuilder& MaterialBuilder::albedoMap(const std::string& filepath) {
  hasAlbedoTexture = true;
  albedoFilepath = filepath;
  return *this;
}

MaterialBuilder& MaterialBuilder::albedoColor(const glm::vec3& color) {
  material.properties.albedoColor = glm::vec4(color, 1.0f);
  return *this;
}

MaterialBuilder& MaterialBuilder::albedoColor(float r, float g, float b) {
  material.properties.albedoColor = glm::vec4(r, g, b, 1.0f);
  return *this;
}

MaterialBuilder& MaterialBuilder::normalMap(TextureID id) {
  material.normalMap = id;
  return *this;
}

MaterialBuilder& MaterialBuilder::normalMap(const std::string& filepath) {
  hasNormalTexture = true;
  normalFilepath = filepath;
  return *this;
}

MaterialBuilder& MaterialBuilder::roughnessMap(TextureID id) {
  material.roughnessMap = id;
  return *this;
}

MaterialBuilder& MaterialBuilder::roughnessMap(const std::string& filepath) {
  hasRoughnessTexture = true;
  roughnessFilepath = filepath;
  return *this;
}

MaterialBuilder& MaterialBuilder::roughness(float value) {
  material.properties.roughness = value;
  return *this;
}

MaterialBuilder& MaterialBuilder::metallicMap(TextureID id) {
  material.metallicMap = id;
  return *this;
}

MaterialBuilder& MaterialBuilder::metallicMap(const std::string& filepath) {
  hasMetallicTexture = true;
  metallicFilepath = filepath;
  return *this;
}

MaterialBuilder& MaterialBuilder::metallic(float value) {
  material.properties.metallic = value;
  return *this;
}

MaterialBuilder& MaterialBuilder::emissiveMap(TextureID id) {
  material.emissiveMap = id;
  return *this;
}

MaterialBuilder& MaterialBuilder::emissiveMap(const std::string& filepath) {
  hasEmissiveTexture = true;
  emissiveFilepath = filepath;
  return *this;
}

MaterialBuilder& MaterialBuilder::emissiveIntensity(float value) {
  material.properties.emissiveIntensity = value;
  return *this;
}

MaterialBuilder& MaterialBuilder::heightMap(TextureID id) {
  material.heightMap = id;
  return *this;
}

MaterialBuilder& MaterialBuilder::heightMap(const std::string& filepath) {
  hasHeightTexture = true;
  heightFilepath = filepath;
  return *this;
}

MaterialBuilder& MaterialBuilder::heightScale(float value) {
  material.properties.heightScale = value;
  return *this;
}

MaterialBuilder& MaterialBuilder::aoMap(TextureID id) {
  material.aoMap = id;
  return *this;
}

MaterialBuilder& MaterialBuilder::aoMap(const std::string& filepath) {
  hasAOTexture = true;
  aoFilepath = filepath;
  return *this;
}

MaterialBuilder& MaterialBuilder::transparent(bool enabled) {
  material.isTransparent = enabled;
  return *this;
}

MaterialBuilder& MaterialBuilder::opacity(float value) {
  material.properties.opacity = value;
  return *this;
}

MaterialBuilder& MaterialBuilder::indexOfRefraction(float ior) {
  material.properties.indexOfRefraction = ior;
  return *this;
}

MaterialBuilder& MaterialBuilder::doubleSided(bool enabled) {
  material.doubleSided = enabled;
  return *this;
}

Material* MaterialBuilder::build() {
  Material* mat = new Material(material);
  return mat;
}