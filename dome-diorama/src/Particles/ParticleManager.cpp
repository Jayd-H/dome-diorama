#include "ParticleManager.h"

#include "Resources/Material.h"
#include "Resources/MaterialManager.h"
#include "Resources/MeshManager.h"
#include "Util/Debug.h"

ParticleManager::ParticleManager(RenderDevice* renderDevice,
                                 MaterialManager* materialManager)
    : renderDevice(renderDevice),
      materialManager(materialManager),
      instanceBuffer(VK_NULL_HANDLE),
      instanceBufferMemory(VK_NULL_HANDLE),
      instanceBufferMapped(nullptr),
      instanceBufferSize(0),
      materialDescriptorSetLayout(VK_NULL_HANDLE),
      particleParamsLayout(VK_NULL_HANDLE),
      particleDescriptorPool(VK_NULL_HANDLE) {
  Debug::log(Debug::Category::PARTICLES, "ParticleManager: Constructor called");
}

ParticleManager::~ParticleManager() {
  Debug::log(Debug::Category::PARTICLES, "ParticleManager: Destructor called");
  cleanup();
}

void ParticleManager::init(VkDescriptorSetLayout materialDescriptorSetLayout,
                           VkPipelineLayout pipelineLayout) {
  Debug::log(Debug::Category::PARTICLES, "ParticleManager: Initializing");

  this->materialDescriptorSetLayout = materialDescriptorSetLayout;

  createParticleDescriptorSetLayout();
  createInstanceBuffer();

  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Initialization complete");
}

EmitterID ParticleManager::registerEmitter(ParticleEmitter* emitter) {
  if (!emitter) {
    Debug::log(Debug::Category::PARTICLES,
               "ParticleManager: Attempted to register null emitter!");
    return INVALID_EMITTER_ID;
  }

  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Registering emitter '", emitter->getName(), "'");

  EmitterID id = static_cast<EmitterID>(emitters.size());
  emitters.push_back(std::unique_ptr<ParticleEmitter>(emitter));

  const size_t frameCount = 2;
  if (shaderParamsBuffers.empty()) {
    createParticleDescriptorPool(frameCount);
  }

  const size_t oldEmitterCount =
      shaderParamsBuffers.empty() ? 0 : shaderParamsBuffers[0].size();

  if (shaderParamsBuffers.empty()) {
    createShaderParamsBuffers(frameCount);
  } else {
    VkDeviceSize bufferSize = sizeof(ParticleShaderParams);
    for (size_t frame = 0; frame < frameCount; ++frame) {
      shaderParamsBuffers[frame].resize(emitters.size());
      shaderParamsMemory[frame].resize(emitters.size());
      shaderParamsMapped[frame].resize(emitters.size());

      renderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 shaderParamsBuffers[frame][id],
                                 shaderParamsMemory[frame][id]);

      vkMapMemory(renderDevice->getDevice(), shaderParamsMemory[frame][id], 0,
                  bufferSize, 0, &shaderParamsMapped[frame][id]);
    }
  }

  createParticleDescriptorSets(id, frameCount);

  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Successfully registered emitter with ID: ", id);

  return id;
}

ParticleEmitter* ParticleManager::getEmitter(EmitterID id) {
  if (id >= emitters.size()) {
    Debug::log(Debug::Category::PARTICLES,
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
}

void ParticleManager::render(VkCommandBuffer commandBuffer,
                             VkDescriptorSet cameraDescriptorSet,
                             VkPipelineLayout pipelineLayout,
                             uint32_t currentFrame, VkPipeline particlePipeline,
                             Mesh* quadMesh) {
  if (emitters.empty()) return;

  if (shaderParamsMapped.empty() || currentFrame >= shaderParamsMapped.size()) {
    Debug::log(Debug::Category::PARTICLES,
               "ParticleManager: Invalid shader params mapped array - size: ",
               shaderParamsMapped.size(), ", currentFrame: ", currentFrame);
    return;
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    particlePipeline);

  VkBuffer vertexBuffers[] = {quadMesh->vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, quadMesh->indexBuffer, 0,
                       VK_INDEX_TYPE_UINT16);

  for (size_t i = 0; i < emitters.size(); ++i) {
    const auto& emitter = emitters[i];
    if (!emitter || !emitter->isActive()) continue;

    if (i >= particleDescriptorSets.size()) {
      Debug::log(Debug::Category::PARTICLES,
                 "ParticleManager: Invalid descriptor set index for emitter ",
                 i);
      continue;
    }

    if (currentFrame >= particleDescriptorSets[i].size()) {
      Debug::log(Debug::Category::PARTICLES,
                 "ParticleManager: Invalid frame index for emitter ", i);
      continue;
    }

    if (i >= shaderParamsMapped[currentFrame].size()) {
      Debug::log(Debug::Category::PARTICLES,
                 "ParticleManager: Invalid shader params index for emitter ",
                 i);
      continue;
    }

    const ParticleShaderParams& params = emitter->getShaderParams();
    memcpy(shaderParamsMapped[currentFrame][i], &params,
           sizeof(ParticleShaderParams));

    Material* material = materialManager->getMaterial(emitter->getMaterialID());
    if (!material) {
      Debug::log(Debug::Category::PARTICLES,
                 "ParticleManager: Invalid material for emitter ", i);
      continue;
    }

    if (material->getDescriptorSet() == VK_NULL_HANDLE) {
      Debug::log(
          Debug::Category::PARTICLES,
          "ParticleManager: Material descriptor set is null for emitter ", i);
      continue;
    }

    std::array<VkDescriptorSet, 3> descriptorSets = {
        cameraDescriptorSet, material->getDescriptorSet(),
        particleDescriptorSets[i][currentFrame]};

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            descriptorSets.data(), 0, nullptr);

    VkDeviceSize instanceOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 1, 1, &instanceBuffer,
                           &instanceOffset);

    size_t particleCount = emitter->getMaxParticles();
    vkCmdDrawIndexed(commandBuffer, 6, static_cast<uint32_t>(particleCount), 0,
                     0, 0);
  }
}

void ParticleManager::cleanup() {
  Debug::log(Debug::Category::PARTICLES, "ParticleManager: Cleaning up");

  if (instanceBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(renderDevice->getDevice(), instanceBuffer, nullptr);
  }
  if (instanceBufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(renderDevice->getDevice(), instanceBufferMemory, nullptr);
  }

  for (size_t frame = 0; frame < shaderParamsBuffers.size(); ++frame) {
    for (size_t emitter = 0; emitter < shaderParamsBuffers[frame].size();
         ++emitter) {
      if (shaderParamsBuffers[frame][emitter] != VK_NULL_HANDLE) {
        vkDestroyBuffer(renderDevice->getDevice(),
                        shaderParamsBuffers[frame][emitter], nullptr);
      }
      if (shaderParamsMemory[frame][emitter] != VK_NULL_HANDLE) {
        vkFreeMemory(renderDevice->getDevice(),
                     shaderParamsMemory[frame][emitter], nullptr);
      }
    }
  }

  if (particleDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(renderDevice->getDevice(), particleDescriptorPool,
                            nullptr);
  }

  if (particleParamsLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(renderDevice->getDevice(),
                                 particleParamsLayout, nullptr);
  }

  emitters.clear();

  Debug::log(Debug::Category::PARTICLES, "ParticleManager: Cleanup complete");
}

void ParticleManager::createInstanceBuffer() {
  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Creating instance buffer");

  instanceBufferSize = sizeof(ParticleInstanceData) * 10000;

  renderDevice->createBuffer(instanceBufferSize,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             instanceBuffer, instanceBufferMemory);

  vkMapMemory(renderDevice->getDevice(), instanceBufferMemory, 0,
              instanceBufferSize, 0, &instanceBufferMapped);

  std::vector<ParticleInstanceData> instanceData(10000);
  for (size_t i = 0; i < 10000; ++i) {
    instanceData[i].particleIndex = static_cast<float>(i);
  }

  memcpy(instanceBufferMapped, instanceData.data(), instanceBufferSize);

  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Instance buffer created");
}

void ParticleManager::createShaderParamsBuffers(size_t frameCount) {
  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Creating shader params buffers");

  VkDeviceSize bufferSize = sizeof(ParticleShaderParams);

  shaderParamsBuffers.clear();
  shaderParamsMemory.clear();
  shaderParamsMapped.clear();

  shaderParamsBuffers.resize(frameCount);
  shaderParamsMemory.resize(frameCount);
  shaderParamsMapped.resize(frameCount);

  for (size_t frame = 0; frame < frameCount; ++frame) {
    shaderParamsBuffers[frame].resize(emitters.size());
    shaderParamsMemory[frame].resize(emitters.size());
    shaderParamsMapped[frame].resize(emitters.size());

    for (size_t emitter = 0; emitter < emitters.size(); ++emitter) {
      renderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 shaderParamsBuffers[frame][emitter],
                                 shaderParamsMemory[frame][emitter]);

      vkMapMemory(renderDevice->getDevice(), shaderParamsMemory[frame][emitter],
                  0, bufferSize, 0, &shaderParamsMapped[frame][emitter]);
    }
  }

  Debug::log(Debug::Category::PARTICLES, "ParticleManager: Created ",
             frameCount * emitters.size(), " shader params buffers");
}

void ParticleManager::createParticleDescriptorSetLayout() {
  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Creating particle descriptor set layout");

  VkDescriptorSetLayoutBinding paramsBinding{};
  paramsBinding.binding = 0;
  paramsBinding.descriptorCount = 1;
  paramsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  paramsBinding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &paramsBinding;

  if (vkCreateDescriptorSetLayout(renderDevice->getDevice(), &layoutInfo,
                                  nullptr,
                                  &particleParamsLayout) != VK_SUCCESS) {
    throw std::runtime_error(
        "Failed to create particle descriptor set layout!");
  }

  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Particle descriptor set layout created");
}

void ParticleManager::createParticleDescriptorPool(size_t frameCount) {
  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Creating particle descriptor pool");

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = static_cast<uint32_t>(frameCount * 10);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(frameCount * 10);

  if (vkCreateDescriptorPool(renderDevice->getDevice(), &poolInfo, nullptr,
                             &particleDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create particle descriptor pool!");
  }

  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Particle descriptor pool created");
}

void ParticleManager::createParticleDescriptorSets(size_t emitterIndex,
                                                   size_t frameCount) {
  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Creating particle descriptor sets for emitter ",
             emitterIndex);

  if (particleDescriptorSets.size() <= emitterIndex) {
    particleDescriptorSets.resize(emitterIndex + 1);
  }

  particleDescriptorSets[emitterIndex].resize(frameCount);

  std::vector<VkDescriptorSetLayout> layouts(frameCount, particleParamsLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = particleDescriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(frameCount);
  allocInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(renderDevice->getDevice(), &allocInfo,
                               particleDescriptorSets[emitterIndex].data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate particle descriptor sets!");
  }

  for (size_t i = 0; i < frameCount; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = shaderParamsBuffers[i][emitterIndex];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(ParticleShaderParams);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = particleDescriptorSets[emitterIndex][i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(renderDevice->getDevice(), 1, &descriptorWrite, 0,
                           nullptr);
  }

  Debug::log(Debug::Category::PARTICLES,
             "ParticleManager: Particle descriptor sets created");
}