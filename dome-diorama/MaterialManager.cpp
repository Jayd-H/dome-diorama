#include "MaterialManager.h"

#include <array>

#include "Debug.h"

MaterialManager::MaterialManager(RenderDevice* renderDevice,
                                 TextureManager* textureManager)
    : renderDevice(renderDevice),
      textureManager(textureManager),
      descriptorSetLayout(VK_NULL_HANDLE),
      descriptorPool(VK_NULL_HANDLE),
      defaultMaterialID(0) {
  Debug::log(Debug::Category::RENDERING, "MaterialManager: Constructor called");
}

MaterialManager::~MaterialManager() {
  Debug::log(Debug::Category::RENDERING, "MaterialManager: Destructor called");
  cleanup();
}

void MaterialManager::init(VkDescriptorSetLayout descriptorSetLayout,
                           VkDescriptorPool descriptorPool) {
  Debug::log(
      Debug::Category::RENDERING,
      "MaterialManager: Initializing with descriptor set layout and pool");

  this->descriptorSetLayout = descriptorSetLayout;
  this->descriptorPool = descriptorPool;

  createDefaultMaterial();

  Debug::log(Debug::Category::RENDERING,
             "MaterialManager: Initialized with default material ID: ",
             defaultMaterialID);
}

MaterialID MaterialManager::registerMaterial(Material* material) {
  if (!material) {
    Debug::log(Debug::Category::RENDERING,
               "MaterialManager: Attempted to register null material!");
    return defaultMaterialID;
  }

  Debug::log(Debug::Category::RENDERING,
             "MaterialManager: Registering material '", material->name, "'");

  if (material->albedoMap == INVALID_TEXTURE_ID) {
    material->albedoMap = textureManager->getDefaultWhite();
    Debug::log(Debug::Category::RENDERING,
               "  - Using default white for albedo map");
  }
  if (material->normalMap == INVALID_TEXTURE_ID) {
    material->normalMap = textureManager->getDefaultNormal();
    Debug::log(Debug::Category::RENDERING,
               "  - Using default normal for normal map");
  }
  if (material->roughnessMap == INVALID_TEXTURE_ID) {
    material->roughnessMap = textureManager->getDefaultWhite();
    Debug::log(Debug::Category::RENDERING,
               "  - Using default white for roughness map");
  }
  if (material->metallicMap == INVALID_TEXTURE_ID) {
    material->metallicMap = textureManager->getDefaultBlack();
    Debug::log(Debug::Category::RENDERING,
               "  - Using default black for metallic map");
  }
  if (material->emissiveMap == INVALID_TEXTURE_ID) {
    material->emissiveMap = textureManager->getDefaultBlack();
    Debug::log(Debug::Category::RENDERING,
               "  - Using default black for emissive map");
  }
  if (material->heightMap == INVALID_TEXTURE_ID) {
    material->heightMap = textureManager->getDefaultBlack();
    Debug::log(Debug::Category::RENDERING,
               "  - Using default black for height map");
  }
  if (material->aoMap == INVALID_TEXTURE_ID) {
    material->aoMap = textureManager->getDefaultWhite();
    Debug::log(Debug::Category::RENDERING,
               "  - Using default white for AO map");
  }

  createDescriptorSet(material);

  MaterialID id = static_cast<MaterialID>(materials.size());
  materials.push_back(std::unique_ptr<Material>(material));

  Debug::log(Debug::Category::RENDERING,
             "MaterialManager: Successfully registered material '",
             material->name, "' with ID: ", id);

  return id;
}

Material* MaterialManager::getMaterial(MaterialID id) {
  if (id >= materials.size()) {
    Debug::log(Debug::Category::RENDERING,
               "MaterialManager: Invalid material ID requested: ", id,
               ", returning default");
    return materials[defaultMaterialID].get();
  }
  return materials[id].get();
}

const Material* MaterialManager::getMaterial(MaterialID id) const {
  if (id >= materials.size()) {
    Debug::log(Debug::Category::RENDERING,
               "MaterialManager: Invalid material ID requested (const): ", id,
               ", returning default");
    return materials[defaultMaterialID].get();
  }
  return materials[id].get();
}

void MaterialManager::updateMaterialProperties(
    MaterialID id, const MaterialProperties& properties) {
  if (id >= materials.size()) {
    Debug::log(Debug::Category::RENDERING,
               "MaterialManager: Cannot update invalid material ID: ", id);
    return;
  }

  Debug::log(Debug::Category::RENDERING,
             "MaterialManager: Updating properties for material ID: ", id);

  materials[id]->properties = properties;
  updateDescriptorSet(materials[id].get());

  Debug::log(Debug::Category::RENDERING,
             "MaterialManager: Successfully updated material ID: ", id);
}

void MaterialManager::cleanup() {
  Debug::log(Debug::Category::RENDERING, "MaterialManager: Cleaning up ",
             materials.size(), " materials");
  materials.clear();
  Debug::log(Debug::Category::RENDERING, "MaterialManager: Cleanup complete");
}

void MaterialManager::createDescriptorSet(Material* material) {
  Debug::log(Debug::Category::RENDERING,
             "MaterialManager: Creating descriptor set for material '",
             material->name, "'");

  VkDevice device = renderDevice->getDevice();
  VkDeviceSize bufferSize = sizeof(MaterialProperties);

  renderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             material->propertiesBuffer,
                             material->propertiesBufferMemory);

  void* data;
  vkMapMemory(device, material->propertiesBufferMemory, 0, bufferSize, 0,
              &data);
  memcpy(data, &material->properties, bufferSize);
  vkUnmapMemory(device, material->propertiesBufferMemory);

  Debug::log(Debug::Category::RENDERING, "  - Created properties buffer");

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;

  if (vkAllocateDescriptorSets(device, &allocInfo, &material->descriptorSet) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor set for material!");
  }

  Debug::log(Debug::Category::RENDERING, "  - Allocated descriptor set");

  updateDescriptorSet(material);

  Debug::log(Debug::Category::RENDERING, "  - Updated descriptor set bindings");
}

void MaterialManager::updateDescriptorSet(Material* material) {
  VkDevice device = renderDevice->getDevice();

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = material->propertiesBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(MaterialProperties);

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

  std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet = material->descriptorSet;
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo = &bufferInfo;

  for (size_t i = 0; i < 7; i++) {
    descriptorWrites[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[i + 1].dstSet = material->descriptorSet;
    descriptorWrites[i + 1].dstBinding = static_cast<uint32_t>(i + 1);
    descriptorWrites[i + 1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[i + 1].descriptorCount = 1;
    descriptorWrites[i + 1].pImageInfo = &imageInfos[i];
  }

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

void MaterialManager::createDefaultMaterial() {
  Debug::log(Debug::Category::RENDERING,
             "MaterialManager: Creating default material");

  Material* defaultMat = new Material();
  defaultMat->name = "Default Material";
  defaultMat->properties.albedoColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
  defaultMat->properties.roughness = 0.5f;
  defaultMat->properties.metallic = 0.0f;

  defaultMaterialID = registerMaterial(defaultMat);

  Debug::log(
      Debug::Category::RENDERING,
      "MaterialManager: Default material created with ID: ", defaultMaterialID);
}