#pragma once
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "ParticleEmitter.h"
#include "RenderDevice.h"

class MaterialManager;

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

  void cleanup();

 private:
  RenderDevice* renderDevice;
  MaterialManager* materialManager;

  std::vector<std::unique_ptr<ParticleEmitter>> emitters;

  VkBuffer instanceBuffer;
  VkDeviceMemory instanceBufferMemory;
  void* instanceBufferMapped;
  VkDeviceSize instanceBufferSize;

  VkDescriptorSetLayout materialDescriptorSetLayout;

  void createInstanceBuffer();
  void updateInstanceBuffer();
};