#pragma once
#include <memory>
#include <vector>

#include "Resources/Material.h"
#include "Rendering/RenderDevice.h"
#include "Resources/TextureManager.h"

class MaterialManager {
 public:
  MaterialManager(RenderDevice* renderDevice, TextureManager* textureManager);
  ~MaterialManager();

  void init(VkDescriptorSetLayout descriptorSetLayout,
            VkDescriptorPool descriptorPool);

  MaterialID registerMaterial(Material* material);
  MaterialID registerMaterial(MaterialBuilder& builder);
  Material* getMaterial(MaterialID id);
  const Material* getMaterial(MaterialID id) const;

  MaterialID getDefaultMaterial() const { return defaultMaterialID; }

  void updateMaterialProperties(MaterialID id,
                                const MaterialProperties& properties);

  void cleanup();

  MaterialID loadFromMTL(const std::string& mtlFilepath);

 private:
  RenderDevice* renderDevice;
  TextureManager* textureManager;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;

  std::unordered_map<std::string, MaterialID> mtlFilepathToID;

  std::vector<std::unique_ptr<Material>> materials;
  MaterialID defaultMaterialID;

  std::unordered_map<std::string, MaterialID> materialNameToID;

  void createDescriptorSet(Material* material);
  void updateDescriptorSet(Material* material);
  void createDefaultMaterial();
};