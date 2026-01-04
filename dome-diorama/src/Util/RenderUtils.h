#pragma once
#include <vulkan/vulkan.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace RenderUtils {

inline void readFile(const std::string& filename, std::vector<char>& buffer) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file: " + filename);
  }

  const size_t fileSize = static_cast<size_t>(file.tellg());
  buffer.resize(fileSize);

  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
  file.close();
}

inline VkShaderModule createShaderModule(VkDevice device,
                                         const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }
  return shaderModule;
}

// Fixed OPT.33: Pass by reference instead of returning by value
// Restored Name: createGraphicsPipelineCreateInfo
inline void createGraphicsPipelineCreateInfo(
    VkGraphicsPipelineCreateInfo& pipelineInfo, VkPipelineLayout layout,
    VkRenderPass renderPass, uint32_t stageCount,
    const VkPipelineShaderStageCreateInfo* pStages,
    const VkPipelineVertexInputStateCreateInfo* pVertexInputState,
    const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
    const VkPipelineViewportStateCreateInfo* pViewportState,
    const VkPipelineRasterizationStateCreateInfo* pRasterizationState,
    const VkPipelineMultisampleStateCreateInfo* pMultisampleState,
    const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState,
    const VkPipelineColorBlendStateCreateInfo* pColorBlendState,
    const VkPipelineDynamicStateCreateInfo* pDynamicState,
    const VkPipelineRenderingCreateInfo* pNextRenderingInfo = nullptr) {
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = pNextRenderingInfo;
  pipelineInfo.stageCount = stageCount;
  pipelineInfo.pStages = pStages;
  pipelineInfo.pVertexInputState = pVertexInputState;
  pipelineInfo.pInputAssemblyState = pInputAssemblyState;
  pipelineInfo.pViewportState = pViewportState;
  pipelineInfo.pRasterizationState = pRasterizationState;
  pipelineInfo.pMultisampleState = pMultisampleState;
  pipelineInfo.pDepthStencilState = pDepthStencilState;
  pipelineInfo.pColorBlendState = pColorBlendState;
  pipelineInfo.pDynamicState = pDynamicState;
  pipelineInfo.layout = layout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;
}

// Fixed OPT.33: Pass by reference instead of returning by value
// Restored Name: createInputAssemblyState
inline void createInputAssemblyState(
    VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
    VkPrimitiveTopology topology) {
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.pNext = nullptr;
  inputAssembly.flags = 0;
  inputAssembly.topology = topology;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
}

// Fixed OPT.33: Pass by reference instead of returning by value
// Restored Name: createColorBlendAttachment
inline void createColorBlendAttachment(
    VkPipelineColorBlendAttachmentState& colorBlendAttachment) {
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

// Fixed OPT.33: Pass by reference instead of returning by value
// Restored Name: createSamplerLayoutBinding
inline void createSamplerLayoutBinding(
    VkDescriptorSetLayoutBinding& samplerLayoutBinding, uint32_t binding,
    VkShaderStageFlags stageFlags) {
  samplerLayoutBinding.binding = binding;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = stageFlags;
}

}  // namespace RenderUtils