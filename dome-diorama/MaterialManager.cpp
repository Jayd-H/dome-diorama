#include "MaterialManager.h"

#include <array>
#include <iostream>

MaterialManager::MaterialManager(VkDevice device,
                                 TextureManager* textureManager)
    : device(device), textureManager(textureManager) {}

MaterialManager::~MaterialManager() { cleanup(); }

void MaterialManager::init(VkDescriptorSetLayout descriptorSetLayout,
                           VkDescriptorPool descriptorPool) {
  this->descriptorSetLayout = descriptorSetLayout;
  this->descriptorPool = descriptorPool;

  createDefaultMaterial();

  std::cout << "MaterialManager: Initialized with default material"
            << std::endl;
}

MaterialID MaterialManager::registerMaterial(Material* material) {
  if (!material) {
    std::cerr << "MaterialManager: Attempted to register null material!"
              << std::endl;
    return defaultMaterialID;
  }

  if (material->albedoMap == INVALID_TEXTURE_ID) {
    material->albedoMap = textureManager->getDefaultWhite();
  }
  if (material->normalMap == INVALID_TEXTURE_ID) {
    material->normalMap = textureManager->getDefaultNormal();
  }
  if (material->roughnessMap == INVALID_TEXTURE_ID) {
    material->roughnessMap = textureManager->getDefaultWhite();
  }
  if (material->metallicMap == INVALID_TEXTURE_ID) {
    material->metallicMap = textureManager->getDefaultBlack();
  }
  if (material->emissiveMap == INVALID_TEXTURE_ID) {
    material->emissiveMap = textureManager->getDefaultBlack();
  }
  if (material->heightMap == INVALID_TEXTURE_ID) {
    material->heightMap = textureManager->getDefaultBlack();
  }
  if (material->aoMap == INVALID_TEXTURE_ID) {
    material->aoMap = textureManager->getDefaultWhite();
  }

  createDescriptorSet(material);

  MaterialID id = static_cast<MaterialID>(materials.size());
  materials.push_back(std::unique_ptr<Material>(material));

  std::cout << "MaterialManager: Registered material '" << material->name
            << "' (ID: " << id << ")" << std::endl;

  return id;
}

Material* MaterialManager::getMaterial(MaterialID id) {
  if (id >= materials.size()) {
    std::cerr << "MaterialManager: Invalid material ID: " << id << std::endl;
    return materials[defaultMaterialID].get();
  }
  return materials[id].get();
}

const Material* MaterialManager::getMaterial(MaterialID id) const {
  if (id >= materials.size()) {
    return materials[defaultMaterialID].get();
  }
  return materials[id].get();
}

void MaterialManager::updateMaterialProperties(
    MaterialID id, const MaterialProperties& properties) {
  if (id >= materials.size()) {
    std::cerr << "MaterialManager: Cannot update invalid material ID: " << id
              << std::endl;
    return;
  }

  materials[id]->properties = properties;
  updateDescriptorSet(materials[id].get());

  std::cout << "MaterialManager: Updated properties for material ID: " << id
            << std::endl;
}

void MaterialManager::cleanup() {
  materials.clear();
  std::cout << "MaterialManager: Cleaned up all materials" << std::endl;
}

void MaterialManager::createDescriptorSet(Material* material) {
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;

  if (vkAllocateDescriptorSets(device, &allocInfo, &material->descriptorSet) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor set for material!");
  }

  updateDescriptorSet(material);
}

void MaterialManager::updateDescriptorSet(Material* material) {
  std::array<VkDescriptorImageInfo, 7> imageInfos{};

  imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[0].imageView = textureManager->getImageView(material->albedoMap);
  imageInfos[0].sampler = textureManager->getSampler(material->albedoMap);

  imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[1].imageView = textureManager->getImageView(material->normalMap);
  imageInfos[1].sampler = textureManager->getSampler(material->normalMap);

  imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[2].imageView =
      textureManager->getImageView(material->roughnessMap);
  imageInfos[2].sampler = textureManager->getSampler(material->roughnessMap);

  imageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[3].imageView = textureManager->getImageView(material->metallicMap);
  imageInfos[3].sampler = textureManager->getSampler(material->metallicMap);

  imageInfos[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[4].imageView = textureManager->getImageView(material->emissiveMap);
  imageInfos[4].sampler = textureManager->getSampler(material->emissiveMap);

  imageInfos[5].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[5].imageView = textureManager->getImageView(material->heightMap);
  imageInfos[5].sampler = textureManager->getSampler(material->heightMap);

  imageInfos[6].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[6].imageView = textureManager->getImageView(material->aoMap);
  imageInfos[6].sampler = textureManager->getSampler(material->aoMap);

  std::array<VkWriteDescriptorSet, 7> descriptorWrites{};

  for (size_t i = 0; i < 7; i++) {
    descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[i].dstSet = material->descriptorSet;
    descriptorWrites[i].dstBinding = static_cast<uint32_t>(i);
    descriptorWrites[i].dstArrayElement = 0;
    descriptorWrites[i].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[i].descriptorCount = 1;
    descriptorWrites[i].pImageInfo = &imageInfos[i];
  }

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

void MaterialManager::createDefaultMaterial() {
  Material* defaultMat = new Material();
  defaultMat->name = "Default Material";
  defaultMat->properties.albedoColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
  defaultMat->properties.roughness = 0.5f;
  defaultMat->properties.metallic = 0.0f;

  defaultMaterialID = registerMaterial(defaultMat);
}