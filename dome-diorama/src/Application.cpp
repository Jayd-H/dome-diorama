#include "Application.h"

#include <array>
#include <chrono>
#include <iostream>

#include "Particles/DustEmitter.h"
#include "Particles/FireEmitter.h"
#include "Particles/RainEmitter.h"
#include "Particles/SmokeEmitter.h"
#include "Util/Debug.h"
#include "Vulkan/VulkanCommandBuffer.h"
#include "Vulkan/VulkanDepthBuffer.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanInstance.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanSyncObjects.h"

Application::Application() {
  Debug::log(Debug::Category::MAIN, "Application: Constructor called");
}

Application::~Application() {
  Debug::log(Debug::Category::MAIN, "Application: Destructor called");
}

void Application::init() {
  initWindow();
  initVulkan();
}

void Application::run() {
  mainLoop();
  cleanup();
}

void Application::initWindow() {
  window = std::make_unique<Window>(WIDTH, HEIGHT, "Dome Diorama");
  window->setUserPointer(this);
  window->setFramebufferSizeCallback(framebufferResizeCallback);
  window->setKeyCallback(keyCallback);
  window->setCursorPosCallback(cursorPosCallback);
  window->setMouseButtonCallback(mouseButtonCallback);
  window->setScrollCallback(scrollCallback);

  lastFrameTime = static_cast<float>(glfwGetTime());
}

void Application::initVulkan() {
  Debug::log(Debug::Category::VULKAN, "Creating instance...");
  instance = Vulkan::createInstance();

  Debug::log(Debug::Category::VULKAN, "Setting up debug messenger...");
  debugMessenger = Vulkan::setupDebugMessenger(instance);

  Debug::log(Debug::Category::VULKAN, "Creating surface...");
  surface = window->createSurface(instance);

  Debug::log(Debug::Category::VULKAN, "Picking physical device...");
  physicalDevice = Vulkan::pickPhysicalDevice(instance, surface);

  Debug::log(Debug::Category::VULKAN, "Creating logical device...");
  device = Vulkan::createLogicalDevice(physicalDevice, surface, graphicsQueue,
                                       presentQueue);

  Debug::log(Debug::Category::VULKAN, "Creating swap chain...");
  swapChain = Vulkan::createSwapChain(device, physicalDevice, surface,
                                      window->getHandle(), swapChainImageFormat,
                                      swapChainExtent, swapChainImages);

  Debug::log(Debug::Category::VULKAN, "Creating image views...");
  Vulkan::createImageViews(device, swapChainImages, swapChainImageFormat,
                           swapChainImageViews);

  Debug::log(Debug::Category::VULKAN, "Finding depth format...");
  depthFormat = Vulkan::findDepthFormat(physicalDevice);
  Debug::log(Debug::Category::VULKAN, "Depth format: ", depthFormat);

  Debug::log(Debug::Category::VULKAN, "Creating descriptor set layout...");
  descriptorSetLayout = Vulkan::createDescriptorSetLayout(device);

  Debug::log(Debug::Category::VULKAN,
             "Creating material descriptor set layout...");
  materialDescriptorSetLayout =
      Vulkan::createMaterialDescriptorSetLayout(device);

  Debug::log(Debug::Category::VULKAN, "Creating command pool...");
  commandPool = Vulkan::createCommandPool(device, physicalDevice, surface);

  Debug::log(Debug::Category::VULKAN, "Creating render device...");
  renderDevice =
      new RenderDevice(device, physicalDevice, commandPool, graphicsQueue);

  Debug::log(Debug::Category::VULKAN, "Creating texture manager...");
  textureManager =
      new TextureManager(device, physicalDevice, commandPool, graphicsQueue);

  Debug::log(Debug::Category::VULKAN, "Creating material manager...");
  materialManager = new MaterialManager(renderDevice, textureManager);

  Debug::log(Debug::Category::VULKAN, "Creating mesh manager...");
  meshManager = new MeshManager(renderDevice);

  Debug::log(Debug::Category::VULKAN, "Creating particle quad mesh...");
  particleQuadMesh = meshManager->createParticleQuad();

  Debug::log(Debug::Category::VULKAN, "Creating light manager...");
  lightManager = new LightManager(renderDevice);

  Debug::log(Debug::Category::VULKAN, "Creating plant manager...");
  plantManager = new PlantManager(meshManager, materialManager);

  Debug::log(Debug::Category::VULKAN, "Creating descriptor pool...");
  descriptorPool = Vulkan::createDescriptorPool(device, MAX_FRAMES_IN_FLIGHT);

  Debug::log(Debug::Category::VULKAN, "Initializing material manager...");
  materialManager->init(materialDescriptorSetLayout, descriptorPool);

  Debug::log(Debug::Category::VULKAN, "Initializing light manager...");
  lightManager->init();

  Debug::log(Debug::Category::VULKAN, "Initializing plant manager...");
  plantManager->init();

  Debug::log(Debug::Category::VULKAN, "Creating particle manager...");
  particleManager = new ParticleManager(renderDevice, materialManager);
  Debug::log(Debug::Category::VULKAN, "Initializing particle manager...");
  particleManager->init(materialDescriptorSetLayout, VK_NULL_HANDLE);

  Debug::log(Debug::Category::VULKAN, "Creating main pipeline...");
  mainPipeline =
      std::make_unique<MainPipeline>(device, swapChainImageFormat, depthFormat);
  mainPipeline->create(descriptorSetLayout, materialDescriptorSetLayout,
                       lightManager->getShadowDescriptorSetLayout());

  Debug::log(Debug::Category::VULKAN, "Creating particle pipeline...");
  createParticlePipeline();

  Debug::log(Debug::Category::VULKAN, "Creating shadow pipeline...");
  createShadowPipeline();

  Debug::log(Debug::Category::VULKAN, "Creating post-processing...");
  postProcessing =
      new PostProcessing(renderDevice, device, swapChainImageFormat);
  postProcessing->init(descriptorPool, swapChainExtent.width,
                       swapChainExtent.height);

  Debug::log(Debug::Category::VULKAN, "Creating depth resources...");
  createDepthResources();

  Debug::log(Debug::Category::VULKAN, "Creating skybox...");
  skybox = new Skybox(renderDevice, device, commandPool, graphicsQueue);
  skybox->init("./Models/Skybox", descriptorSetLayout, swapChainImageFormat,
               depthFormat);

  Debug::log(Debug::Category::VULKAN, "Creating uniform buffers...");
  createUniformBuffers();

  Debug::log(Debug::Category::VULKAN, "Creating descriptor sets...");
  Vulkan::createDescriptorSets(device, descriptorPool, descriptorSetLayout,
                               uniformBuffers, lightManager->getLightBuffer(),
                               MAX_FRAMES_IN_FLIGHT, descriptorSets);

  Debug::log(Debug::Category::VULKAN, "Creating command buffers...");
  commandBuffers =
      Vulkan::createCommandBuffers(device, commandPool, MAX_FRAMES_IN_FLIGHT);

  Debug::log(Debug::Category::VULKAN, "Creating sync objects...");
  Vulkan::createSyncObjects(device, MAX_FRAMES_IN_FLIGHT,
                            imageAvailableSemaphores, renderFinishedSemaphores,
                            inFlightFences);

  Debug::log(Debug::Category::VULKAN, "Vulkan initialization complete!");
}

void Application::mainLoop() {
  Debug::log(Debug::Category::MAIN, "Entering main loop...");

  while (!window->shouldClose()) {
    window->pollEvents();

    const float currentTime = static_cast<float>(glfwGetTime());
    const float deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

    input.update();
    camera.update(input, deltaTime);
    camera.setCursorMode(window->getHandle());
    input.endFrame();

    drawFrame();
  }

  Debug::log(Debug::Category::MAIN, "Exiting main loop...");
  vkDeviceWaitIdle(device);
}

void Application::cleanup() {
  vkDeviceWaitIdle(device);

  cleanupSwapChain();

  if (skybox) {
    skybox->cleanup();
    delete skybox;
  }

  if (postProcessing) {
    postProcessing->cleanup();
    delete postProcessing;
  }

  if (lightManager) {
    lightManager->cleanup();
    delete lightManager;
  }
  if (plantManager) {
    delete plantManager;
  }
  if (materialManager) {
    materialManager->cleanup();
    delete materialManager;
  }
  if (textureManager) {
    textureManager->cleanup();
    delete textureManager;
  }
  if (renderDevice) {
    delete renderDevice;
  }
  if (meshManager) {
    meshManager->cleanup();
    delete meshManager;
  }
  if (particleManager) {
    particleManager->cleanup();
    delete particleManager;
  }
  if (shadowPipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(device, shadowPipeline, nullptr);
  }
  if (shadowPipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, shadowPipelineLayout, nullptr);
  }

  if (mainPipeline) {
    mainPipeline->cleanup();
  }

  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, materialDescriptorSetLayout, nullptr);

  vkDestroyPipeline(device, particlePipeline, nullptr);
  vkDestroyPipelineLayout(device, particlePipelineLayout, nullptr);

  vkDestroyBuffer(device, indexBuffer, nullptr);
  vkFreeMemory(device, indexBufferMemory, nullptr);
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
  }
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyDevice(device, nullptr);

  if (Vulkan::enableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
    Vulkan::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
}

void Application::createUniformBuffers() {
  const VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    renderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               uniformBuffers[i], uniformBuffersMemory[i]);
    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0,
                &uniformBuffersMapped[i]);
  }
}

void Application::drawFrame() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
      VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swap chain image!");
  }

  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  updateUniformBuffer(currentFrame);

  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  const std::array<VkSemaphore, 1> waitSemaphores = {
      imageAvailableSemaphores[currentFrame]};
  const std::array<VkPipelineStageFlags, 1> waitStages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

  const std::array<VkSemaphore, 1> signalSemaphores = {
      renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores.data();

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                    inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("Failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores.data();

  const std::array<VkSwapchainKHR, 1> swapChains = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains.data();
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::recreateSwapChain() {
  int width = 0, height = 0;
  window->getFramebufferSize(width, height);
  while (width == 0 || height == 0) {
    window->getFramebufferSize(width, height);
    window->waitEvents();
  }

  vkDeviceWaitIdle(device);

  cleanupSwapChain();

  swapChain = Vulkan::createSwapChain(device, physicalDevice, surface,
                                      window->getHandle(), swapChainImageFormat,
                                      swapChainExtent, swapChainImages);
  Vulkan::createImageViews(device, swapChainImages, swapChainImageFormat,
                           swapChainImageViews);

  createDepthResources();
  postProcessing->resize(swapChainExtent.width, swapChainExtent.height,
                         descriptorPool);
}

void Application::cleanupSwapChain() {
  if (depthImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(device, depthImageView, nullptr);
    depthImageView = VK_NULL_HANDLE;
  }
  if (depthImage != VK_NULL_HANDLE) {
    vkDestroyImage(device, depthImage, nullptr);
    depthImage = VK_NULL_HANDLE;
  }
  if (depthImageMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, depthImageMemory, nullptr);
    depthImageMemory = VK_NULL_HANDLE;
  }

  for (const auto imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void Application::updateUniformBuffer(uint32_t currentImage) {
  static const auto startTime = std::chrono::high_resolution_clock::now();
  const auto currentTime = std::chrono::high_resolution_clock::now();
  const float time =
      std::chrono::duration<float>(currentTime - startTime).count();

  const float deltaTime = time - (time - 0.016f);

  worldState.update(deltaTime);
  particleManager->update(deltaTime);

  const glm::vec3 sunDirection = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
  const float sunOrbitRadius = 150.0f;
  const glm::vec3 sunPosition = -sunDirection * sunOrbitRadius;
  sceneObjects[0].setPosition(sunPosition);

  Light* const sunLight = lightManager->getLight(sunLightID);
  if (sunLight) {
    sunLight->direction = sunDirection;
    sunLight->intensity = 5.0f;
    sunLight->color = glm::vec3(1.0f, 0.95f, 0.85f);

    const glm::vec3 sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    const glm::vec3 lightEye = sceneCenter - sunDirection * 500.0f;

    const glm::mat4 lightView =
        glm::lookAt(lightEye, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));

    const float orthoSize = 800.0f;
    glm::mat4 lightProjection =
        glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 1000.0f);

    lightProjection[1][1] *= -1;

    const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    static bool matrixLogged = false;
    if (!matrixLogged) {
      Debug::log(Debug::Category::SHADOWS,
                 "Light space matrix column 0: ", lightSpaceMatrix[0][0], ", ",
                 lightSpaceMatrix[0][1], ", ", lightSpaceMatrix[0][2], ", ",
                 lightSpaceMatrix[0][3]);
      Debug::log(Debug::Category::SHADOWS,
                 "Light space matrix column 1: ", lightSpaceMatrix[1][0], ", ",
                 lightSpaceMatrix[1][1], ", ", lightSpaceMatrix[1][2], ", ",
                 lightSpaceMatrix[1][3]);
      matrixLogged = true;
    }

    if (sunLight->shadowMapIndex != UINT32_MAX) {
      lightManager->getShadowSystem()->updateLightSpaceMatrix(
          sunLight->shadowMapIndex, lightSpaceMatrix);
    }

    static bool shadowMapLogged = false;
    if (!shadowMapLogged) {
      const auto& shadowMaps = lightManager->getShadowSystem()->getShadowMaps();
      Debug::log(Debug::Category::SHADOWS, "Shadow map 0 matrix column 0: ",
                 shadowMaps[0].lightSpaceMatrix[0][0], ", ",
                 shadowMaps[0].lightSpaceMatrix[0][1], ", ",
                 shadowMaps[0].lightSpaceMatrix[0][2], ", ",
                 shadowMaps[0].lightSpaceMatrix[0][3]);
      shadowMapLogged = true;
    }

    static bool logged = false;
    if (!logged) {
      Debug::log(Debug::Category::SHADOWS, "Light eye position: ", lightEye.x,
                 ", ", lightEye.y, ", ", lightEye.z);
      Debug::log(Debug::Category::SHADOWS, "Scene center: ", sceneCenter.x,
                 ", ", sceneCenter.y, ", ", sceneCenter.z);
      Debug::log(Debug::Category::SHADOWS, "Sun direction: ", sunDirection.x,
                 ", ", sunDirection.y, ", ", sunDirection.z);
      logged = true;
    }
  }

  lightManager->updateLightBuffer();

  UniformBufferObject ubo{};
  ubo.view = camera.getViewMatrix();
  ubo.proj = glm::perspective(
      glm::radians(45.0f),
      swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f,
      50000.0f);
  ubo.proj[1][1] *= -1;

  ubo.eyePos = camera.getPosition();
  ubo.time = time;

  const auto& shadowMaps = lightManager->getShadowSystem()->getShadowMaps();
  for (size_t i = 0; i < shadowMaps.size() && i < MAX_SHADOW_CASTERS; i++) {
    ubo.lightSpaceMatrices[i] = shadowMaps[i].lightSpaceMatrix;
  }
  for (size_t i = shadowMaps.size(); i < MAX_SHADOW_CASTERS; i++) {
    ubo.lightSpaceMatrices[i] = glm::mat4(1.0f);
  }

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Application::recreateGraphicsPipeline() {
  vkDeviceWaitIdle(device);
  mainPipeline->recreate();
}

void Application::recreateTextureSamplers(VkFilter magFilter,
                                          VkFilter minFilter) {
  vkDeviceWaitIdle(device);
  textureManager->recreateSamplers(magFilter, minFilter);
  Debug::log(Debug::Category::RENDERING,
             "Recreated texture samplers with filter mode: ",
             magFilter == VK_FILTER_NEAREST ? "NEAREST" : "LINEAR");
}

void Application::toggleShadingMode() {
  if (mainPipeline->getShadingMode() == MainPipeline::ShadingMode::Phong) {
    mainPipeline->setShadingMode(MainPipeline::ShadingMode::Gouraud);
    window->setTitle("Dome Diorama - GOURAUD SHADING");
    Debug::log(Debug::Category::INPUT, "Switched to GOURAUD shading");
  } else {
    mainPipeline->setShadingMode(MainPipeline::ShadingMode::Phong);
    window->setTitle("Dome Diorama - PHONG SHADING");
    Debug::log(Debug::Category::INPUT, "Switched to PHONG shading");
  }
  recreateGraphicsPipeline();
}

void Application::createDepthResources() {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = swapChainExtent.width;
  imageInfo.extent.height = swapChainExtent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = depthFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create depth image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, depthImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = renderDevice->findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate depth image memory!");
  }

  vkBindImageMemory(device, depthImage, depthImageMemory, 0);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = depthImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = depthFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create depth image view!");
  }
}

void Application::framebufferResizeCallback(GLFWwindow* win, int width,
                                            int height) {
  auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->framebufferResized = true;
}

void Application::keyCallback(GLFWwindow* win, int key, int scancode,
                              int action, int mods) {
  if (win == nullptr) return;
  auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->input.onKey(key, scancode, action, mods);

  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_1) {
      app->mainPipeline->setPolygonMode(VK_POLYGON_MODE_FILL);
      app->recreateGraphicsPipeline();
      app->window->setTitle("Dome Diorama - FILL MODE");
      Debug::log(Debug::Category::INPUT, "Switched to FILL mode");
    } else if (key == GLFW_KEY_2) {
      app->mainPipeline->setPolygonMode(VK_POLYGON_MODE_LINE);
      app->recreateGraphicsPipeline();
      app->window->setTitle("Dome Diorama - WIREFRAME MODE");
      Debug::log(Debug::Category::INPUT, "Switched to WIREFRAME mode");
    } else if (key == GLFW_KEY_3) {
      app->mainPipeline->setPolygonMode(VK_POLYGON_MODE_POINT);
      app->recreateGraphicsPipeline();
      app->window->setTitle("Dome Diorama - POINT MODE");
      Debug::log(Debug::Category::INPUT, "Switched to POINT mode");
    } else if (key == GLFW_KEY_4) {
      app->recreateTextureSamplers(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
      app->window->setTitle("Dome Diorama - NEAREST FILTERING");
      Debug::log(Debug::Category::INPUT, "Switched to NEAREST filtering");
    } else if (key == GLFW_KEY_5) {
      app->recreateTextureSamplers(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
      app->window->setTitle("Dome Diorama - LINEAR FILTERING");
      Debug::log(Debug::Category::INPUT, "Switched to LINEAR filtering");
    } else if (key == GLFW_KEY_L) {
      app->toggleShadingMode();
    }
  }
}

void Application::cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
  auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->input.onMouseMove(xpos, ypos);
}

void Application::mouseButtonCallback(GLFWwindow* win, int button, int action,
                                      int mods) {
  auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->input.onMouseButton(button, action, mods);
}

void Application::scrollCallback(GLFWwindow* win, double xoffset,
                                 double yoffset) {
  auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->input.onScroll(xoffset, yoffset);
}

void Application::createParticlePipeline() {
  Debug::log(Debug::Category::VULKAN, "Creating particle pipeline...");

  const auto vertShaderCode = readFile("shaders/particle_vert.spv");
  const auto fragShaderCode = readFile("shaders/particle_frag.spv");

  const VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  const VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
      vertShaderStageInfo, fragShaderStageInfo};

  const auto bindingDescription = Vertex::getBindingDescription();

  VkVertexInputBindingDescription instanceBinding{};
  instanceBinding.binding = 1;
  instanceBinding.stride = sizeof(ParticleInstanceData);
  instanceBinding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

  const std::array<VkVertexInputBindingDescription, 2> bindings = {
      bindingDescription, instanceBinding};

  std::array<VkVertexInputAttributeDescription, 5> attributes{};
  attributes[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)};
  attributes[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)};
  attributes[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)};
  attributes[3] = {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
  attributes[4] = {4, 1, VK_FORMAT_R32_SFLOAT,
                   offsetof(ParticleInstanceData, particleIndex)};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindings.size());
  vertexInputInfo.pVertexBindingDescriptions = bindings.data();
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributes.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_FALSE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  std::array<VkDescriptorSetLayout, 3> layouts = {
      descriptorSetLayout, materialDescriptorSetLayout,
      particleManager->getParticleParamsLayout()};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &particlePipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create particle pipeline layout!");
  }

  VkPipelineRenderingCreateInfo renderingCreateInfo{};
  renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingCreateInfo.colorAttachmentCount = 1;
  renderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;
  renderingCreateInfo.depthAttachmentFormat = depthFormat;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = &renderingCreateInfo;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = particlePipelineLayout;
  pipelineInfo.renderPass = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &particlePipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create particle graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);

  Debug::log(Debug::Category::VULKAN, "Particle pipeline created successfully");
}

void Application::createShadowPipeline() {
  Debug::log(Debug::Category::VULKAN, "Creating shadow pipeline...");

  const auto vertShaderCode = readFile("shaders/shadow_vert.spv");
  const auto fragShaderCode = readFile("shaders/shadow_frag.spv");

  const VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  const VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
      vertShaderStageInfo, fragShaderStageInfo};

  const auto bindingDescription = Vertex::getBindingDescription();
  const auto attributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 1;
  vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptions[0];

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_TRUE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.attachmentCount = 0;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR,
                                               VK_DYNAMIC_STATE_DEPTH_BIAS};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  struct ShadowPushConstants {
    glm::mat4 lightSpaceMatrix;
    glm::mat4 model;
  };

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(ShadowPushConstants);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &shadowPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow pipeline layout!");
  }

  const VkFormat shadowDepthFormat = VK_FORMAT_D32_SFLOAT;

  VkPipelineRenderingCreateInfo renderingCreateInfo{};
  renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingCreateInfo.depthAttachmentFormat = shadowDepthFormat;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = &renderingCreateInfo;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = shadowPipelineLayout;
  pipelineInfo.renderPass = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &shadowPipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shadow graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);

  Debug::log(Debug::Category::VULKAN, "Shadow pipeline created successfully");
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                      uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("Failed to begin recording command buffer!");
  }

  const auto& shadowMaps = lightManager->getShadowSystem()->getShadowMaps();

  for (size_t smIdx = 0; smIdx < shadowMaps.size(); smIdx++) {
    const auto& shadowMap = shadowMaps[smIdx];

    VkImageMemoryBarrier2 toWriteBarrier{};
    toWriteBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toWriteBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    toWriteBarrier.srcAccessMask = VK_ACCESS_2_NONE;
    toWriteBarrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    toWriteBarrier.dstAccessMask =
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    toWriteBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toWriteBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    toWriteBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toWriteBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toWriteBarrier.image = shadowMap.image;
    toWriteBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    toWriteBarrier.subresourceRange.baseMipLevel = 0;
    toWriteBarrier.subresourceRange.levelCount = 1;
    toWriteBarrier.subresourceRange.baseArrayLayer = 0;
    toWriteBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo toWriteDep{};
    toWriteDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    toWriteDep.imageMemoryBarrierCount = 1;
    toWriteDep.pImageMemoryBarriers = &toWriteBarrier;
    vkCmdPipelineBarrier2(commandBuffer, &toWriteDep);

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = shadowMap.imageView;
    depthAttachment.imageLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      shadowPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdSetDepthBias(commandBuffer, 0.0f, 0.0f, 0.0f);

    struct ShadowPushConstants {
      glm::mat4 lightSpaceMatrix;
      glm::mat4 model;
    } shadowPush;

    shadowPush.lightSpaceMatrix = shadowMap.lightSpaceMatrix;

    int objectsRendered = 0;
    for (const auto& object : sceneObjects) {
      if (!object.visible) continue;
      if (object.meshID == INVALID_MESH_ID) continue;

      const Mesh* mesh = meshManager->getMesh(object.meshID);
      if (!mesh || mesh->vertexBuffer == VK_NULL_HANDLE) continue;

      shadowPush.model = object.getModelMatrix();

      vkCmdPushConstants(commandBuffer, shadowPipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0,
                         sizeof(ShadowPushConstants), &shadowPush);

      VkBuffer vertexBuffers[] = {mesh->vertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
      vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0,
                           VK_INDEX_TYPE_UINT16);

      vkCmdDrawIndexed(commandBuffer,
                       static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
      objectsRendered++;
    }

    vkCmdEndRendering(commandBuffer);

    VkImageMemoryBarrier2 toReadBarrier{};
    toReadBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toReadBarrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    toReadBarrier.srcAccessMask =
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    toReadBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    toReadBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    toReadBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    toReadBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    toReadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toReadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toReadBarrier.image = shadowMap.image;
    toReadBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    toReadBarrier.subresourceRange.baseMipLevel = 0;
    toReadBarrier.subresourceRange.levelCount = 1;
    toReadBarrier.subresourceRange.baseArrayLayer = 0;
    toReadBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo toReadDep{};
    toReadDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    toReadDep.imageMemoryBarrierCount = 1;
    toReadDep.pImageMemoryBarriers = &toReadBarrier;
    vkCmdPipelineBarrier2(commandBuffer, &toReadDep);
  }

  postProcessing->beginOffscreenPass(commandBuffer, depthImageView,
                                     swapChainExtent);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    mainPipeline->getPipeline());

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapChainExtent.width);
  viewport.height = static_cast<float>(swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  for (const auto& object : sceneObjects) {
    if (!object.visible) continue;
    if (object.meshID == INVALID_MESH_ID) continue;
    if (object.materialID == INVALID_MATERIAL_ID) continue;

    const Mesh* mesh = meshManager->getMesh(object.meshID);
    const Material* material = materialManager->getMaterial(object.materialID);

    if (!mesh || mesh->vertexBuffer == VK_NULL_HANDLE) continue;
    if (!material || material->getDescriptorSet() == VK_NULL_HANDLE) continue;

    const glm::mat4 modelMatrix = object.getModelMatrix();
    vkCmdPushConstants(
        commandBuffer, mainPipeline->getPipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(glm::mat4), &modelMatrix);

    VkBuffer vertexBuffers[] = {mesh->vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0,
                         VK_INDEX_TYPE_UINT16);

    std::array<VkDescriptorSet, 3> descriptorSetsToBind = {
        descriptorSets[currentFrame], material->getDescriptorSet(),
        lightManager->getShadowDescriptorSet()};

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mainPipeline->getPipelineLayout(), 0,
                            static_cast<uint32_t>(descriptorSetsToBind.size()),
                            descriptorSetsToBind.data(), 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()),
                     1, 0, 0, 0);
  }

  const Object* domeGlassObject = &sceneObjects[3];
  skybox->render(commandBuffer, descriptorSets[currentFrame], swapChainExtent,
                 domeGlassObject);

  particleManager->render(
      commandBuffer, descriptorSets[currentFrame], particlePipelineLayout,
      currentFrame, particlePipeline, meshManager->getMesh(particleQuadMesh));

  postProcessing->endOffscreenPass(commandBuffer);

  VkImageMemoryBarrier2 swapchainBarrier{};
  swapchainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  swapchainBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
  swapchainBarrier.srcAccessMask = 0;
  swapchainBarrier.dstStageMask =
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  swapchainBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  swapchainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  swapchainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  swapchainBarrier.image = swapChainImages[imageIndex];
  swapchainBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers = &swapchainBarrier;

  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  postProcessing->render(commandBuffer, swapChainImageViews[imageIndex],
                         swapChainExtent, currentFrame);

  swapchainBarrier.srcStageMask =
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  swapchainBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  swapchainBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  swapchainBarrier.dstAccessMask = 0;
  swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to record command buffer!");
  }
}

VkShaderModule Application::createShaderModule(
    const std::vector<char>& code) const {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
  return shaderModule;
}

std::vector<char> Application::readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }
  const size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
  file.close();
  return buffer;
}