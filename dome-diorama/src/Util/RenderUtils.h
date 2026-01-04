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

inline VkGraphicsPipelineCreateInfo createGraphicsPipelineCreateInfo(
    VkPipelineLayout layout, VkRenderPass renderPass, uint32_t stageCount,
    const VkPipelineShaderStageCreateInfo* pStages,
    const VkPipelineVertexInputStateCreateInfo* pVertexInputState,
    const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
    const VkPipelineViewportStateCreateInfo* pViewportState,
    const VkPipelineRasterizationStateCreateInfo* pRasterizationState,
    const VkPipelineMultisampleStateCreateInfo* pMultisampleState,
    const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState,
    const VkPipelineColorBlendStateCreateInfo* pColorBlendState,
    const VkPipelineDynamicStateCreateInfo* pDynamicState,
    const void* pNext = nullptr) {
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = pNext;
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

  return pipelineInfo;
}

inline VkPipelineInputAssemblyStateCreateInfo createInputAssemblyState(
    VkPrimitiveTopology topology) {
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = topology;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
  return inputAssembly;
}

inline VkPipelineColorBlendAttachmentState createColorBlendAttachment() {
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  return colorBlendAttachment;
}

inline VkDescriptorSetLayoutBinding createSamplerLayoutBinding(
    uint32_t binding, VkShaderStageFlags stageFlags) {
  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = binding;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.stageFlags = stageFlags;
  return samplerLayoutBinding;
}

}  // namespace RenderUtils