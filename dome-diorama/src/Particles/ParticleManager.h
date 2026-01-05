#pragma once
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "Particles/ParticleEmitter.h"
#include "Rendering/RenderDevice.h"

class MaterialManager;
class Mesh;

class ParticleManager final {
 public:
  ParticleManager(RenderDevice* rd, MaterialManager* mm);
  ~ParticleManager();

  ParticleManager(const ParticleManager&) = delete;
  ParticleManager& operator=(const ParticleManager&) = delete;

  void init(VkDescriptorSetLayout matDescriptorSetLayout);

  EmitterID registerEmitter(ParticleEmitter* emitter);
  ParticleEmitter* getEmitter(EmitterID id);

  void update(float deltaTime, const glm::vec3& windDirection, float windSpeed);
  void render(VkCommandBuffer commandBuffer,
              VkDescriptorSet cameraDescriptorSet,
              VkPipelineLayout pipelineLayout, uint32_t currentFrame,
              VkPipeline particlePipeline, Mesh* const quadMesh);

  VkDescriptorSetLayout getParticleParamsLayout() const {
    return particleParamsLayout;
  }

  void cleanup();

 private:
  RenderDevice* renderDevice;
  MaterialManager* materialManager;

  VkBuffer instanceBuffer;
  VkDeviceMemory instanceBufferMemory;
  void* instanceBufferMapped;
  VkDeviceSize instanceBufferSize;

  VkDescriptorSetLayout materialDescriptorSetLayout;
  VkDescriptorSetLayout particleParamsLayout;
  VkDescriptorPool particleDescriptorPool;

  std::vector<std::vector<VkBuffer>> shaderParamsBuffers;
  std::vector<std::vector<VkDeviceMemory>> shaderParamsMemory;
  std::vector<std::vector<void*>> shaderParamsMapped;
  std::vector<std::vector<VkDescriptorSet>> particleDescriptorSets;
  std::vector<std::unique_ptr<ParticleEmitter>> emitters;

  void createInstanceBuffer();
  void createShaderParamsBuffers(size_t frameCount);
  void createParticleDescriptorSetLayout();
  void createParticleDescriptorPool(size_t frameCount);
  void createParticleDescriptorSets(size_t emitterIndex, size_t frameCount);
};
