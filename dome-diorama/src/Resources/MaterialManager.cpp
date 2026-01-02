#include "MaterialManager.h"

#include <array>
#include <fstream>
#include <sstream>

#include "Util/Debug.h"

MaterialManager::MaterialManager(RenderDevice* renderDevice,
                                 TextureManager* textureManager)
    : renderDevice(renderDevice),
      textureManager(textureManager),
      descriptorSetLayout(VK_NULL_HANDLE),
      descriptorPool(VK_NULL_HANDLE),
      defaultMaterialID(0) {
  Debug::log(Debug::Category::MATERIALS, "MaterialManager: Constructor called");
}

MaterialManager::~MaterialManager() {
  Debug::log(Debug::Category::MATERIALS, "MaterialManager: Destructor called");
  cleanup();
}

void MaterialManager::init(VkDescriptorSetLayout descriptorSetLayout,
                           VkDescriptorPool descriptorPool) {
  Debug::log(
      Debug::Category::MATERIALS,
      "MaterialManager: Initializing with descriptor set layout and pool");

  this->descriptorSetLayout = descriptorSetLayout;
  this->descriptorPool = descriptorPool;

  createDefaultMaterial();

  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: Initialized with default material ID: ",
             defaultMaterialID);
}

MaterialID MaterialManager::registerMaterial(Material* material) {
  if (!material) {
    Debug::log(Debug::Category::MATERIALS,
               "MaterialManager: Attempted to register null material!");
    return defaultMaterialID;
  }

  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: Registering material '", material->getName(),
             "'");

  if (material->getAlbedoMap() == INVALID_TEXTURE_ID) {
    material->setAlbedoMap(textureManager->getDefaultWhite());
    Debug::log(Debug::Category::MATERIALS,
               "  - Using default white for albedo map");
  }
  if (material->getNormalMap() == INVALID_TEXTURE_ID) {
    material->setNormalMap(textureManager->getDefaultNormal());
    Debug::log(Debug::Category::MATERIALS,
               "  - Using default normal for normal map");
  }
  if (material->getRoughnessMap() == INVALID_TEXTURE_ID) {
    material->setRoughnessMap(textureManager->getDefaultWhite());
    Debug::log(Debug::Category::MATERIALS,
               "  - Using default white for roughness map");
  }
  if (material->getMetallicMap() == INVALID_TEXTURE_ID) {
    material->setMetallicMap(textureManager->getDefaultBlack());
    Debug::log(Debug::Category::MATERIALS,
               "  - Using default black for metallic map");
  }
  if (material->getEmissiveMap() == INVALID_TEXTURE_ID) {
    material->setEmissiveMap(textureManager->getDefaultBlack());
    Debug::log(Debug::Category::MATERIALS,
               "  - Using default black for emissive map");
  }
  if (material->getHeightMap() == INVALID_TEXTURE_ID) {
    material->setHeightMap(textureManager->getDefaultBlack());
    Debug::log(Debug::Category::MATERIALS,
               "  - Using default black for height map");
  }
  if (material->getAoMap() == INVALID_TEXTURE_ID) {
    material->setAoMap(textureManager->getDefaultWhite());
    Debug::log(Debug::Category::MATERIALS,
               "  - Using default white for AO map");
  }

  createDescriptorSet(material);

  MaterialID id = static_cast<MaterialID>(materials.size());
  materials.push_back(std::unique_ptr<Material>(material));

  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: Successfully registered material '",
             material->getName(), "' with ID: ", id);

  return id;
}

MaterialID MaterialManager::registerMaterial(MaterialBuilder& builder) {
  Material* material = builder.build();

  if (builder.getHasAlbedoTexture()) {
    material->setAlbedoMap(
        textureManager->load(builder.getAlbedoFilepath(), TextureType::sRGB));
  }
  if (builder.getHasNormalTexture()) {
    material->setNormalMap(
        textureManager->load(builder.getNormalFilepath(), TextureType::Linear));
  }
  if (builder.getHasRoughnessTexture()) {
    material->setRoughnessMap(textureManager->load(
        builder.getRoughnessFilepath(), TextureType::Linear));
  }
  if (builder.getHasMetallicTexture()) {
    material->setMetallicMap(textureManager->load(builder.getMetallicFilepath(),
                                                  TextureType::Linear));
  }
  if (builder.getHasEmissiveTexture()) {
    material->setEmissiveMap(
        textureManager->load(builder.getEmissiveFilepath(), TextureType::sRGB));
  }
  if (builder.getHasHeightTexture()) {
    material->setHeightMap(
        textureManager->load(builder.getHeightFilepath(), TextureType::Linear));
  }
  if (builder.getHasAOTexture()) {
    material->setAoMap(
        textureManager->load(builder.getAoFilepath(), TextureType::Linear));
  }

  return registerMaterial(material);
}

Material* MaterialManager::getMaterial(MaterialID id) {
  if (id >= materials.size()) {
    Debug::log(Debug::Category::MATERIALS,
               "MaterialManager: Invalid material ID requested: ", id,
               ", returning default");
    return materials[defaultMaterialID].get();
  }
  return materials[id].get();
}

const Material* MaterialManager::getMaterial(MaterialID id) const {
  if (id >= materials.size()) {
    Debug::log(Debug::Category::MATERIALS,
               "MaterialManager: Invalid material ID requested (const): ", id,
               ", returning default");
    return materials[defaultMaterialID].get();
  }
  return materials[id].get();
}

void MaterialManager::updateMaterialProperties(
    MaterialID id, const MaterialProperties& properties) {
  if (id >= materials.size()) {
    Debug::log(Debug::Category::MATERIALS,
               "MaterialManager: Cannot update invalid material ID: ", id);
    return;
  }

  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: Updating properties for material ID: ", id);

  materials[id]->setProperties(properties);
  updateDescriptorSet(materials[id].get());

  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: Successfully updated material ID: ", id);
}

void MaterialManager::cleanup() {
  Debug::log(Debug::Category::MATERIALS, "MaterialManager: Cleaning up ",
             materials.size(), " materials");
  materials.clear();
  Debug::log(Debug::Category::MATERIALS, "MaterialManager: Cleanup complete");
}

void MaterialManager::createDescriptorSet(Material* material) {
  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: Creating descriptor set for material '",
             material->getName(), "'");

  VkDevice device = renderDevice->getDevice();
  VkDeviceSize bufferSize = sizeof(MaterialProperties);

  VkBuffer buffer;
  VkDeviceMemory memory;

  renderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             buffer, memory);

  material->setPropertiesBuffer(buffer);
  material->setPropertiesBufferMemory(memory);

  void* data;
  vkMapMemory(device, memory, 0, bufferSize, 0, &data);
  memcpy(data, &material->getProperties(), bufferSize);
  vkUnmapMemory(device, memory);

  Debug::log(Debug::Category::MATERIALS, "  - Created properties buffer");

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;

  VkDescriptorSet set;
  if (vkAllocateDescriptorSets(device, &allocInfo, &set) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor set for material!");
  }

  material->setDescriptorSet(set);

  Debug::log(Debug::Category::MATERIALS, "  - Allocated descriptor set");

  updateDescriptorSet(material);

  Debug::log(Debug::Category::MATERIALS, "  - Updated descriptor set bindings");
}

void MaterialManager::updateDescriptorSet(Material* material) {
  VkDevice device = renderDevice->getDevice();

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = material->getPropertiesBuffer();
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(MaterialProperties);

  std::array<VkDescriptorImageInfo, 7> imageInfos{};

  imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[0].imageView =
      textureManager->getImageView(material->getAlbedoMap());
  imageInfos[0].sampler = textureManager->getSampler(material->getAlbedoMap());

  imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[1].imageView =
      textureManager->getImageView(material->getNormalMap());
  imageInfos[1].sampler = textureManager->getSampler(material->getNormalMap());

  imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[2].imageView =
      textureManager->getImageView(material->getRoughnessMap());
  imageInfos[2].sampler =
      textureManager->getSampler(material->getRoughnessMap());

  imageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[3].imageView =
      textureManager->getImageView(material->getMetallicMap());
  imageInfos[3].sampler =
      textureManager->getSampler(material->getMetallicMap());

  imageInfos[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[4].imageView =
      textureManager->getImageView(material->getEmissiveMap());
  imageInfos[4].sampler =
      textureManager->getSampler(material->getEmissiveMap());

  imageInfos[5].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[5].imageView =
      textureManager->getImageView(material->getHeightMap());
  imageInfos[5].sampler = textureManager->getSampler(material->getHeightMap());

  imageInfos[6].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfos[6].imageView = textureManager->getImageView(material->getAoMap());
  imageInfos[6].sampler = textureManager->getSampler(material->getAoMap());

  std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet = material->getDescriptorSet();
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo = &bufferInfo;

  for (size_t i = 0; i < 7; i++) {
    descriptorWrites[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[i + 1].dstSet = material->getDescriptorSet();
    descriptorWrites[i + 1].dstBinding = static_cast<uint32_t>(i + 1);
    descriptorWrites[i + 1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[i + 1].descriptorCount = 1;
    descriptorWrites[i + 1].pImageInfo = &imageInfos[i];
  }

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

MaterialID MaterialManager::loadFromMTL(const std::string& mtlFilepath) {
  auto it = mtlFilepathToID.find(mtlFilepath);
  if (it != mtlFilepathToID.end()) {
    Debug::log(Debug::Category::MATERIALS,
               "MaterialManager: MTL already loaded: ", mtlFilepath,
               " (ID: ", it->second, ")");
    return it->second;
  }

  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: Loading MTL: ", mtlFilepath);

  std::ifstream file(mtlFilepath);
  if (!file.is_open()) {
    Debug::log(Debug::Category::MATERIALS,
               "MaterialManager: Failed to open MTL file: ", mtlFilepath,
               ", returning default material");
    return defaultMaterialID;
  }

  std::string baseDir =
      mtlFilepath.substr(0, mtlFilepath.find_last_of("/\\") + 1);
  std::vector<MaterialID> loadedMaterials;
  Material* currentMaterial = nullptr;
  std::string currentMaterialName;

  auto extractFilename = [](const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    return (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
  };

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "newmtl") {
      if (currentMaterial) {
        auto nameIt = materialNameToID.find(currentMaterialName);
        if (nameIt != materialNameToID.end()) {
          Debug::log(Debug::Category::MATERIALS, "  - Material '",
                     currentMaterialName,
                     "' already exists with ID: ", nameIt->second, ", reusing");
          loadedMaterials.push_back(nameIt->second);
          delete currentMaterial;
        } else {
          MaterialID id = registerMaterial(currentMaterial);
          loadedMaterials.push_back(id);
          materialNameToID[currentMaterialName] = id;
          mtlFilepathToID[mtlFilepath + "::" + currentMaterialName] = id;
        }
      }

      iss >> currentMaterialName;

      auto nameIt = materialNameToID.find(currentMaterialName);
      if (nameIt != materialNameToID.end()) {
        Debug::log(Debug::Category::MATERIALS, "  - Material '",
                   currentMaterialName,
                   "' already exists, will reuse ID: ", nameIt->second);
        loadedMaterials.push_back(nameIt->second);
        currentMaterial = nullptr;
        continue;
      }

      currentMaterial = new Material();
      currentMaterial->setName(currentMaterialName);

      Debug::log(Debug::Category::MATERIALS,
                 "  - Found material: ", currentMaterialName);

    } else if (currentMaterial) {
      if (prefix == "Ns") {
        float ns;
        iss >> ns;
        currentMaterial->getPropertiesMutable().roughness =
            1.0f - (ns / 1000.0f);
      } else if (prefix == "Ka") {
      } else if (prefix == "Kd") {
        float r, g, b;
        iss >> r >> g >> b;
        currentMaterial->getPropertiesMutable().albedoColor =
            glm::vec4(r, g, b, 1.0f);
      } else if (prefix == "Ks") {
        float r, g, b;
        iss >> r >> g >> b;
        float specular = (r + g + b) / 3.0f;
        currentMaterial->getPropertiesMutable().metallic = specular;
      } else if (prefix == "Ke") {
        float r, g, b;
        iss >> r >> g >> b;
        float emission = (r + g + b) / 3.0f;
        currentMaterial->getPropertiesMutable().emissiveIntensity = emission;
      } else if (prefix == "Ni") {
        float ior;
        iss >> ior;
        currentMaterial->getPropertiesMutable().indexOfRefraction = ior;
      } else if (prefix == "d") {
        float opacity;
        iss >> opacity;
        currentMaterial->getPropertiesMutable().opacity = opacity;
        if (opacity < 1.0f) {
          currentMaterial->setIsTransparent(true);
        }
      } else if (prefix == "illum") {
      } else if (prefix == "map_Kd") {
        std::string texPath;
        std::getline(iss, texPath);
        texPath = texPath.substr(texPath.find_first_not_of(" \t"));

        std::string filename = extractFilename(texPath);
        std::string fullPath = baseDir + "textures/" + filename;
        TextureID texID = textureManager->load(fullPath, TextureType::sRGB);
        currentMaterial->setAlbedoMap(texID);

        Debug::log(Debug::Category::MATERIALS,
                   "    - Loaded albedo texture: ", fullPath);
      } else if (prefix == "map_Ks") {
        std::string texPath;
        std::getline(iss, texPath);
        texPath = texPath.substr(texPath.find_first_not_of(" \t"));

        std::string filename = extractFilename(texPath);
        std::string fullPath = baseDir + "textures/" + filename;
        TextureID texID = textureManager->load(fullPath, TextureType::Linear);
        currentMaterial->setMetallicMap(texID);

        Debug::log(Debug::Category::MATERIALS,
                   "    - Loaded specular texture: ", fullPath);
      } else if (prefix == "map_Bump" || prefix == "bump") {
        std::string texPath;
        std::getline(iss, texPath);
        texPath = texPath.substr(texPath.find_first_not_of(" \t"));

        std::string filename = extractFilename(texPath);
        std::string fullPath = baseDir + "textures/" + filename;
        TextureID texID = textureManager->load(fullPath, TextureType::Linear);
        currentMaterial->setNormalMap(texID);

        Debug::log(Debug::Category::MATERIALS,
                   "    - Loaded normal texture: ", fullPath);
      } else if (prefix == "map_d") {
        std::string texPath;
        std::getline(iss, texPath);
        texPath = texPath.substr(texPath.find_first_not_of(" \t"));

        std::string filename = extractFilename(texPath);
        std::string fullPath = baseDir + "textures/" + filename;
        TextureID texID = textureManager->load(fullPath, TextureType::Linear);

        Debug::log(Debug::Category::MATERIALS,
                   "    - Found opacity texture: ", fullPath);

        currentMaterial->setIsTransparent(true);
      }
    }
  }

  if (currentMaterial) {
    auto nameIt = materialNameToID.find(currentMaterialName);
    if (nameIt != materialNameToID.end()) {
      Debug::log(Debug::Category::MATERIALS, "  - Material '",
                 currentMaterialName,
                 "' already exists with ID: ", nameIt->second, ", reusing");
      loadedMaterials.push_back(nameIt->second);
      delete currentMaterial;
    } else {
      MaterialID id = registerMaterial(currentMaterial);
      loadedMaterials.push_back(id);
      materialNameToID[currentMaterialName] = id;
      mtlFilepathToID[mtlFilepath + "::" + currentMaterialName] = id;
    }
  }

  file.close();

  if (!loadedMaterials.empty()) {
    mtlFilepathToID[mtlFilepath] = loadedMaterials[0];
    Debug::log(Debug::Category::MATERIALS, "MaterialManager: Loaded ",
               loadedMaterials.size(),
               " materials from MTL, returning first material ID: ",
               loadedMaterials[0]);
    return loadedMaterials[0];
  }

  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: No materials found in MTL, returning default");
  return defaultMaterialID;
}

void MaterialManager::createDefaultMaterial() {
  Debug::log(Debug::Category::MATERIALS,
             "MaterialManager: Creating default material");

  Material* defaultMat = new Material();
  defaultMat->setName("Default Material");
  defaultMat->getPropertiesMutable().albedoColor =
      glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
  defaultMat->getPropertiesMutable().roughness = 0.5f;
  defaultMat->getPropertiesMutable().metallic = 0.0f;

  defaultMaterialID = registerMaterial(defaultMat);

  Debug::log(
      Debug::Category::MATERIALS,
      "MaterialManager: Default material created with ID: ", defaultMaterialID);
}