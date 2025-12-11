#include "ParticleManager.h"

#include "Debug.h"
#include "Material.h"
#include "MaterialManager.h"
#include "MeshManager.h"

ParticleManager::ParticleManager(RenderDevice* renderDevice,
                                 MaterialManager* materialManager)
    : renderDevice(renderDevice),
      materialManager(materialManager),
      instanceBuffer(VK_NULL_HANDLE),
      instanceBufferMemory(VK_NULL_HANDLE),
      instanceBufferMapped(nullptr),
      instanceBufferSize(0),
      materialDescriptorSetLayout(VK_NULL_HANDLE) {
  Debug::log(Debug::Category::RENDERING, "ParticleManager: Constructor called");
}

ParticleManager::~ParticleManager() {
  Debug::log(Debug::Category::RENDERING, "ParticleManager: Destructor called");
  cleanup();
}

void ParticleManager::init(VkDescriptorSetLayout materialDescriptorSetLayout,
                           VkPipelineLayout pipelineLayout) {
  Debug::log(Debug::Category::RENDERING, "ParticleManager: Initializing");

  this->materialDescriptorSetLayout = materialDescriptorSetLayout;

  createInstanceBuffer();

  Debug::log(Debug::Category::RENDERING,
             "ParticleManager: Initialization complete");
}

EmitterID ParticleManager::registerEmitter(ParticleEmitter* emitter) {
  if (!emitter) {
    Debug::log(Debug::Category::RENDERING,
               "ParticleManager: Attempted to register null emitter!");
    return INVALID_EMITTER_ID;
  }

  Debug::log(Debug::Category::RENDERING,
             "ParticleManager: Registering emitter '", emitter->name, "'");

  EmitterID id = static_cast<EmitterID>(emitters.size());
  emitters.push_back(std::unique_ptr<ParticleEmitter>(emitter));

  Debug::log(Debug::Category::RENDERING,
             "ParticleManager: Successfully registered emitter with ID: ", id);

  return id;
}

ParticleEmitter* ParticleManager::getEmitter(EmitterID id) {
  if (id >= emitters.size()) {
    Debug::log(Debug::Category::RENDERING,
               "ParticleManager: Invalid emitter ID requested: ", id);
    return nullptr;
  }
  return emitters[id].get();
}

void ParticleManager::update(float deltaTime) {
  for (auto& emitter : emitters) {
    if (emitter) {
      emitter->update(deltaTime);
    }
  }

  updateInstanceBuffer();
}

void ParticleManager::render(VkCommandBuffer commandBuffer,
                             VkDescriptorSet cameraDescriptorSet,
                             VkPipelineLayout pipelineLayout,
                             uint32_t currentFrame, VkPipeline particlePipeline,
                             Mesh* quadMesh) {
  if (emitters.empty()) return;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    particlePipeline);

  VkBuffer vertexBuffers[] = {quadMesh->vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, quadMesh->indexBuffer, 0,
                       VK_INDEX_TYPE_UINT16);

  for (const auto& emitter : emitters) {
    if (!emitter || emitter->getActiveParticleCount() == 0) continue;

    Material* material = materialManager->getMaterial(emitter->getMaterialID());

    std::array<VkDescriptorSet, 2> descriptorSets = {cameraDescriptorSet,
                                                     material->descriptorSet};

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            descriptorSets.data(), 0, nullptr);

    vkCmdBindVertexBuffers(commandBuffer, 1, 1, &instanceBuffer, offsets);

    vkCmdDrawIndexed(commandBuffer, 6,
                     static_cast<uint32_t>(emitter->getActiveParticleCount()),
                     0, 0, 0);
  }
}

void ParticleManager::cleanup() {
  Debug::log(Debug::Category::RENDERING, "ParticleManager: Cleaning up");

  if (instanceBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(renderDevice->getDevice(), instanceBuffer, nullptr);
  }
  if (instanceBufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(renderDevice->getDevice(), instanceBufferMemory, nullptr);
  }

  emitters.clear();

  Debug::log(Debug::Category::RENDERING, "ParticleManager: Cleanup complete");
}

void ParticleManager::createInstanceBuffer() {
  Debug::log(Debug::Category::RENDERING,
             "ParticleManager: Creating instance buffer");

  instanceBufferSize = sizeof(ParticleInstanceData) * 10000;

  renderDevice->createBuffer(instanceBufferSize,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             instanceBuffer, instanceBufferMemory);

  vkMapMemory(renderDevice->getDevice(), instanceBufferMemory, 0,
              instanceBufferSize, 0, &instanceBufferMapped);

  Debug::log(Debug::Category::RENDERING,
             "ParticleManager: Instance buffer created");
}

void ParticleManager::updateInstanceBuffer() {
  std::vector<ParticleInstanceData> allInstanceData;

  for (const auto& emitter : emitters) {
    if (!emitter) continue;

    const auto& emitterData = emitter->getInstanceData();
    allInstanceData.insert(allInstanceData.end(), emitterData.begin(),
                           emitterData.end());
  }

  if (allInstanceData.empty()) return;

  VkDeviceSize dataSize = sizeof(ParticleInstanceData) * allInstanceData.size();

  if (dataSize > instanceBufferSize) {
    Debug::log(Debug::Category::RENDERING,
               "ParticleManager: Warning - instance data exceeds buffer size!");
    dataSize = instanceBufferSize;
  }

  memcpy(instanceBufferMapped, allInstanceData.data(),
         static_cast<size_t>(dataSize));
}