#pragma once
#include <memory>
#include <vector>

#include "Material.h"
#include "TextureManager.h"

class MaterialManager {
 public:
  MaterialManager(VkDevice device, TextureManager* textureManager);
  ~MaterialManager();

  void init(VkDescriptorSetLayout descriptorSetLayout,
            VkDescriptorPool descriptorPool);

  MaterialID registerMaterial(Material* material);
  Material* getMaterial(MaterialID id);
  const Material* getMaterial(MaterialID id) const;

  MaterialID getDefaultMaterial() const { return defaultMaterialID; }

  void updateMaterialProperties(MaterialID id,
                                const MaterialProperties& properties);

  void cleanup();

 private:
  VkDevice device;
  TextureManager* textureManager;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;

  std::vector<std::unique_ptr<Material>> materials;
  MaterialID defaultMaterialID;

  void createDescriptorSet(Material* material);
  void updateDescriptorSet(Material* material);
  void createDefaultMaterial();
};