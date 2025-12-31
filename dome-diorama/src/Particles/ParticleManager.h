#pragma once
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "Particles/ParticleEmitter.h"
#include "Rendering/RenderDevice.h"

class MaterialManager;
class Mesh;

class ParticleManager {
 public:
  ParticleManager(RenderDevice* renderDevice, MaterialManager* materialManager);
  ~ParticleManager();

  void init(VkDescriptorSetLayout materialDescriptorSetLayout,
            VkPipelineLayout pipelineLayout);

  EmitterID registerEmitter(ParticleEmitter* emitter);
  ParticleEmitter* getEmitter(EmitterID id);

  void update(float deltaTime);
  void render(VkCommandBuffer commandBuffer,
              VkDescriptorSet cameraDescriptorSet,
              VkPipelineLayout pipelineLayout, uint32_t currentFrame,
              VkPipeline particlePipeline, Mesh* quadMesh);

  VkDescriptorSetLayout getParticleParamsLayout() const {
    return particleParamsLayout;
  }

  void cleanup();

 private:
  RenderDevice* renderDevice;
  MaterialManager* materialManager;

  std::vector<std::unique_ptr<ParticleEmitter>> emitters;

  VkBuffer instanceBuffer;
  VkDeviceMemory instanceBufferMemory;
  void* instanceBufferMapped;
  VkDeviceSize instanceBufferSize;

  std::vector<VkBuffer> shaderParamsBuffers;
  std::vector<VkDeviceMemory> shaderParamsMemory;
  std::vector<void*> shaderParamsMapped;

  VkDescriptorSetLayout materialDescriptorSetLayout;
  VkDescriptorSetLayout particleParamsLayout;
  VkDescriptorPool particleDescriptorPool;
  std::vector<std::vector<VkDescriptorSet>> particleDescriptorSets;

  void createInstanceBuffer();
  void createShaderParamsBuffers(size_t frameCount);
  void createParticleDescriptorSetLayout();
  void createParticleDescriptorPool(size_t frameCount);
  void createParticleDescriptorSets(size_t emitterIndex, size_t frameCount);
};