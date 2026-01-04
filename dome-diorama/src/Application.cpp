#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Application.h"

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>

#include "Particles/ParticleEmitter.h"
#include "Util/Debug.h"
#include "Vulkan/VulkanCommandBuffer.h"
#include "Vulkan/VulkanDepthBuffer.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanInstance.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanSyncObjects.h"
#include "Particles/EmitterTypes.h"

// Private helper to avoid duplication and non-member function issues
namespace {
std::vector<char> readFile(const std::string& filename) {
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
}  // namespace

Application::Application() {
  Debug::log(Debug::Category::MAIN, "Application: Constructor called");
}

Application::~Application() {
  try {
    Debug::log(Debug::Category::MAIN, "Application: Destructor called");
  } catch (...) {
    // Destructors should not throw
  }
}

void Application::init() {
  initWindow();
  initVulkan();
}

void Application::setScene(const std::vector<Object>& objects) {
  sceneObjects = objects;

  plantObjectIndicesSet.clear();
  const size_t count = plantManager->getPlantCount();
  for (size_t i = 0; i < count; ++i) {
    plantObjectIndicesSet.insert(plantManager->getPlantObjectIndex(i));
  }

  Debug::log(Debug::Category::PLANTMANAGER, "Application: Registered ",
             plantObjectIndicesSet.size(), " plant objects for wind rendering");
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
  renderDevice = std::make_unique<RenderDevice>(device, physicalDevice,
                                                commandPool, graphicsQueue);

  Debug::log(Debug::Category::VULKAN, "Creating texture manager...");
  textureManager = std::make_unique<TextureManager>(device, physicalDevice,
                                                    commandPool, graphicsQueue);

  Debug::log(Debug::Category::VULKAN, "Creating material manager...");
  materialManager = std::make_unique<MaterialManager>(renderDevice.get(),
                                                      textureManager.get());

  Debug::log(Debug::Category::VULKAN, "Creating mesh manager...");
  meshManager = std::make_unique<MeshManager>(renderDevice.get());

  Debug::log(Debug::Category::VULKAN, "Creating particle quad mesh...");
  particleQuadMesh = meshManager->createParticleQuad();

  Debug::log(Debug::Category::VULKAN, "Creating light manager...");
  lightManager = std::make_unique<LightManager>(renderDevice.get());

  Debug::log(Debug::Category::VULKAN, "Creating plant manager...");
  plantManager =
      std::make_unique<PlantManager>(meshManager.get(), materialManager.get());

  Debug::log(Debug::Category::VULKAN, "Creating descriptor pool...");
  descriptorPool = Vulkan::createDescriptorPool(device, MAX_FRAMES_IN_FLIGHT);

  Debug::log(Debug::Category::VULKAN, "Initializing material manager...");
  materialManager->init(materialDescriptorSetLayout, descriptorPool);

  Debug::log(Debug::Category::VULKAN, "Initializing light manager...");
  lightManager->init();

  Debug::log(Debug::Category::VULKAN, "Initializing plant manager...");
  plantManager->init();

  Debug::log(Debug::Category::VULKAN, "Creating particle manager...");
  particleManager = std::make_unique<ParticleManager>(renderDevice.get(),
                                                      materialManager.get());
  Debug::log(Debug::Category::VULKAN, "Initializing particle manager...");
  particleManager->init(materialDescriptorSetLayout);

  Debug::log(Debug::Category::VULKAN, "Creating main pipeline...");
  mainPipeline =
      std::make_unique<MainPipeline>(device, swapChainImageFormat, depthFormat);
  mainPipeline->create(descriptorSetLayout, materialDescriptorSetLayout,
                       lightManager->getShadowDescriptorSetLayout());

  Debug::log(Debug::Category::VULKAN, "Creating particle pipeline...");
  createParticlePipeline();

  Debug::log(Debug::Category::VULKAN, "Creating weather system...");
  weatherSystem = std::make_unique<WeatherSystem>(particleManager.get(),
                                                  materialManager.get());

  Debug::log(Debug::Category::VULKAN, "Creating shadow pipeline...");
  createShadowPipeline();

  Debug::log(Debug::Category::VULKAN, "Creating plant pipeline...");
  createPlantPipeline();

  Debug::log(Debug::Category::VULKAN, "Creating post-processing...");
  postProcessing = std::make_unique<PostProcessing>(renderDevice.get(), device,
                                                    swapChainImageFormat);
  postProcessing->init(descriptorPool, swapChainExtent.width,
                       swapChainExtent.height);

  Debug::log(Debug::Category::VULKAN, "Creating depth resources...");
  createDepthResources();

  Debug::log(Debug::Category::VULKAN, "Creating skybox...");
  skybox = std::make_unique<Skybox>(renderDevice.get(), device, commandPool,
                                    graphicsQueue);
  skybox->init("./Models/Skybox", descriptorSetLayout, swapChainImageFormat,
               depthFormat);

  Debug::log(Debug::Category::VULKAN, "Creating uniform buffers...");
  createUniformBuffers();

  Debug::log(Debug::Category::VULKAN, "Creating descriptor sets...");
  Vulkan::createDescriptorSets(device, descriptorPool, descriptorSetLayout,
                               uniformBuffers, lightManager->getLightBuffer(),
                               MAX_FRAMES_IN_FLIGHT, descriptorSets);

  Debug::log(Debug::Category::VULKAN, "Creating command buffers...");
  Vulkan::createCommandBuffers(device, commandPool, MAX_FRAMES_IN_FLIGHT,
                               commandBuffers);

  Debug::log(Debug::Category::VULKAN, "Creating sync objects...");
  Vulkan::createSyncObjects(device, MAX_FRAMES_IN_FLIGHT,
                            imageAvailableSemaphores, renderFinishedSemaphores,
                            inFlightFences);

  Debug::log(Debug::Category::VULKAN, "Vulkan initialization complete!");
}

void Application::mainLoop() {
  Debug::log(Debug::Category::MAIN, "Entering main loop...");

  setCameraPreset(1);

  while (!window->shouldClose()) {
    window->pollEvents();

    const float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

    scaledDeltaTime = deltaTime * timeScale;
    simulationTime += scaledDeltaTime;

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

  skybox.reset();
  postProcessing.reset();
  lightManager.reset();
  plantManager.reset();
  materialManager.reset();
  textureManager.reset();
  renderDevice.reset();
  meshManager.reset();
  particleManager.reset();
  weatherSystem.reset();

  if (shadowPipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(device, shadowPipeline, nullptr);
  }
  if (shadowPipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, shadowPipelineLayout, nullptr);
  }
  if (plantPipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, plantPipelineLayout, nullptr);
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

void Application::updateCameraLayer() {
  const float SKYBOX_RADIUS = 300.0f;
  const glm::vec3 cameraPos = camera.getPosition();
  const float distanceFromOrigin = glm::length(cameraPos);

  if (distanceFromOrigin <= SKYBOX_RADIUS) {
    currentCameraLayer = 0xFFFFFFFF;
  } else {
    currentCameraLayer = 0xFFFFFFFE;
  }
}


void Application::updateUniformBuffer(uint32_t currentImage) {
  worldState.update(scaledDeltaTime);

  EnvironmentConditions conditions;
  conditions.temperature = worldState.getTemperature();
  conditions.humidity = worldState.getHumidity();
  conditions.precipitationIntensity = worldState.getPrecipitationIntensity();
  conditions.windDirection = worldState.getWindDirection();
  conditions.windStrength = worldState.getWindSpeed();
  conditions.deltaTime = scaledDeltaTime;

  plantManager->updateEnvironment(sceneObjects, conditions);

  particleManager->update(scaledDeltaTime, worldState.getWindDirection(),
                          worldState.getWindSpeed());

  if (weatherSystem) {
    weatherSystem->update(worldState, scaledDeltaTime);
  }

  const glm::vec3 sunDirection = worldState.getSunDirection();
  const glm::vec3 moonDirection = worldState.getMoonDirection();

  const TimeOfDay timeOfDay = worldState.getTime();
  const float normalizedTime = timeOfDay.normalizedTime;

  const float sunAngle = normalizedTime * glm::two_pi<float>();
  const float sunRadius = 500.0f;
  const glm::vec3 sunPosition =
      glm::vec3(cos(sunAngle) * sunRadius, sin(sunAngle) * sunRadius, 0.0f);

  const float moonAngle = (normalizedTime + 0.5f) * glm::two_pi<float>();
  const float moonRadius = 450.0f;
  const glm::vec3 moonPosition =
      glm::vec3(cos(moonAngle) * moonRadius, sin(moonAngle) * moonRadius, 0.0f);

  Light* const sunLight = lightManager->getLight(sunLightID);
  if (sunLight) {
    sunLight->direction = -sunDirection;
    const float sunHeight = sunDirection.y;

    if (sunHeight > 0.0f) {
      const float t = glm::smoothstep(0.0f, 0.3f, sunHeight);
      sunLight->color = glm::mix(glm::vec3(1.0f, 0.7f, 0.4f),
                                 glm::vec3(1.0f, 0.98f, 0.95f), t);
      const float normalizedIntensity = glm::smoothstep(0.0f, 0.2f, sunHeight);
      sunLight->intensity = normalizedIntensity * 5.0f;
    } else {
      sunLight->intensity = 0.0f;
      sunLight->color = glm::vec3(0.0f);
    }
  }

  for (auto& object : sceneObjects) {
    if (object.getName() == "Sun") {
      object.setPosition(sunPosition);
    } else if (object.getName() == "Moon") {
      object.setPosition(moonPosition);
    }
  }

  updateCameraLayer();

  const glm::vec3 sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
  const float sceneRadius = 200.0f;
  lightManager->updateAllShadowMatrices(sceneCenter, sceneRadius);

  lightManager->updateLightBuffer();

  UniformBufferObject ubo{};
  ubo.view = camera.getViewMatrix();
  ubo.proj = glm::perspective(
      glm::radians(45.0f),
      swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f,
      50000.0f);
  ubo.proj[1][1] *= -1;

  ubo.eyePos = camera.getPosition();
  ubo.time = simulationTime;

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
  auto* const app =
      reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->framebufferResized = true;
}

void Application::increaseTemperature() {
  worldState.adjustTemperature(5.0f);
  Debug::log(Debug::Category::MAIN,
             "Temperature increased to: ", worldState.getTemperature(), "°C");
}

void Application::decreaseTemperature() {
  worldState.adjustTemperature(-5.0f);
  Debug::log(Debug::Category::MAIN,
             "Temperature decreased to: ", worldState.getTemperature(), "°C");
}

void Application::increaseHumidity() {
  worldState.adjustHumidity(0.1f);
  Debug::log(Debug::Category::MAIN,
             "Humidity increased to: ", worldState.getHumidity() * 100.0f, "%");
}

void Application::decreaseHumidity() {
  worldState.adjustHumidity(-0.1f);
  Debug::log(Debug::Category::MAIN,
             "Humidity decreased to: ", worldState.getHumidity() * 100.0f, "%");
}

void Application::increaseWindSpeed() {
  worldState.adjustWindSpeed(1.0f);
  Debug::log(Debug::Category::MAIN,
             "Wind speed increased to: ", worldState.getWindSpeed(), " m/s");
}

void Application::decreaseWindSpeed() {
  worldState.adjustWindSpeed(-1.0f);
  Debug::log(Debug::Category::MAIN,
             "Wind speed decreased to: ", worldState.getWindSpeed(), " m/s");
}

void Application::cycleWeather() {
  worldState.cycleWeather();
  const WeatherState weather = worldState.getWeather();
  std::string weatherName;
  switch (weather) {
    case WeatherState::Clear:
      weatherName = "Clear";
      break;
    case WeatherState::Cloudy:
      weatherName = "Cloudy";
      break;
    case WeatherState::LightRain:
      weatherName = "Light Rain";
      break;
    case WeatherState::HeavyRain:
      weatherName = "Heavy Rain";
      break;
    case WeatherState::LightSnow:
      weatherName = "Light Snow";
      break;
    case WeatherState::HeavySnow:
      weatherName = "Heavy Snow";
      break;
    case WeatherState::DustStorm:
      weatherName = "Dust Storm";
      break;
  }
  Debug::log(Debug::Category::MAIN, "Weather changed to: ", weatherName);
}

void Application::toggleTimePause() {
  worldState.togglePause();
  Debug::log(Debug::Category::MAIN, "Time ",
             worldState.isPaused() ? "PAUSED" : "RESUMED");
}

void Application::keyCallback(GLFWwindow* win, int key, int scancode,
                              int action, int mods) {
  if (win == nullptr) return;
  auto* const app =
      reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->input.onKey(key, scancode, action, mods);

  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_ESCAPE) {
      glfwSetWindowShouldClose(win, true);
    } else if (key == GLFW_KEY_R) {
      app->resetApplication();
    } else if (key == GLFW_KEY_F1) {
      app->setCameraPreset(1);
    } else if (key == GLFW_KEY_F2) {
      app->setCameraPreset(2);
    } else if (key == GLFW_KEY_F3) {
      app->setCameraPreset(3);
    } else if (key == GLFW_KEY_F4) {
      app->triggerFireEffect();
    } else if (key == GLFW_KEY_RIGHT_BRACKET) {  // Increase Time Scale
      app->timeScale += 0.5f;
      Debug::log(Debug::Category::MAIN,
                 "Time Scale Increased: ", app->timeScale);
    } else if (key == GLFW_KEY_LEFT_BRACKET) {  // Decrease Time Scale
      app->timeScale = std::max(0.0f, app->timeScale - 0.5f);
      Debug::log(Debug::Category::MAIN,
                 "Time Scale Decreased: ", app->timeScale);
    } else if (key == GLFW_KEY_1) {
      app->mainPipeline->setPolygonMode(VK_POLYGON_MODE_FILL);
      app->recreateGraphicsPipeline();
      app->window->setTitle("Dome Diorama - FILL MODE");
    } else if (key == GLFW_KEY_2) {
      app->mainPipeline->setPolygonMode(VK_POLYGON_MODE_LINE);
      app->recreateGraphicsPipeline();
      app->window->setTitle("Dome Diorama - WIREFRAME MODE");
    } else if (key == GLFW_KEY_3) {
      app->mainPipeline->setPolygonMode(VK_POLYGON_MODE_POINT);
      app->recreateGraphicsPipeline();
      app->window->setTitle("Dome Diorama - POINT MODE");
    } else if (key == GLFW_KEY_4) {
      app->recreateTextureSamplers(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
      app->window->setTitle("Dome Diorama - NEAREST FILTERING");
    } else if (key == GLFW_KEY_5) {
      app->recreateTextureSamplers(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
      app->window->setTitle("Dome Diorama - LINEAR FILTERING");
    } else if (key == GLFW_KEY_L) {
      app->toggleShadingMode();
    } else if (key == GLFW_KEY_T) {
      app->increaseTemperature();
    } else if (key == GLFW_KEY_G) {
      app->decreaseTemperature();
    } else if (key == GLFW_KEY_H) {
      app->increaseHumidity();
    } else if (key == GLFW_KEY_N) {
      app->decreaseHumidity();
    } else if (key == GLFW_KEY_U) {
      app->increaseWindSpeed();
    } else if (key == GLFW_KEY_J) {
      app->decreaseWindSpeed();
    } else if (key == GLFW_KEY_Y) {
      app->cycleWeather();
    } else if (key == GLFW_KEY_P) {
      app->toggleTimePause();
    }
  }
}

void Application::setCameraPreset(int presetIndex) {
  if (presetIndex == 1) {
    camera.setPose(glm::vec3(0.0f, 300.0f, 500.0f),
                   glm::vec3(0.0f, 0.0f, 0.0f));
    Debug::log(Debug::Category::CAMERA, "Camera C1: Overview");
  } else if (presetIndex == 2) {
    camera.setPose(glm::vec3(50.0f, 30.0f, 50.0f),
                   glm::vec3(0.0f, 10.0f, 0.0f));
    Debug::log(Debug::Category::CAMERA, "Camera C2: Navigation");
  } else if (presetIndex == 3) {
    for (size_t i = 0; i < plantManager->getPlantCount(); ++i) {
      const Plant& p = plantManager->getPlant(i);
      if (p.getType() == PlantType::Cactus) {
        const Object& obj = sceneObjects[plantManager->getPlantObjectIndex(i)];
        glm::vec3 plantPos = obj.getPosition();
        camera.setPose(plantPos + glm::vec3(5.0f, 2.0f, 5.0f),
                       plantPos + glm::vec3(0.0f, 2.0f, 0.0f));
        Debug::log(Debug::Category::CAMERA, "Camera C3: Close-up on Cactus");
        break;
      }
    }
  }
}

void Application::triggerFireEffect() {
  std::vector<size_t> validCacti;
  for (size_t i = 0; i < plantManager->getPlantCount(); ++i) {
    const Plant& p = plantManager->getPlant(i);
    if (p.getType() == PlantType::Cactus && !p.state_.isOnFire &&
        !p.state_.isDead) {
      validCacti.push_back(i);
    }
  }

  if (validCacti.empty()) return;

  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0,
                                      static_cast<int>(validCacti.size()) - 1);
  size_t selectedIndex = validCacti[dis(gen)];

  Plant& p = plantManager->getPlantMutable(selectedIndex);
  const Object& obj =
      sceneObjects[plantManager->getPlantObjectIndex(selectedIndex)];
  glm::vec3 pos = obj.getPosition();

  plantManager->startFire(p, pos);

  glm::vec3 camPos = pos + glm::vec3(5.0f, 5.0f, 5.0f);
  camera.setPose(camPos, pos);

  Debug::log(Debug::Category::MAIN, "F4: Fire started on cactus index ",
             selectedIndex);
}

void Application::resetApplication() {
  timeScale = 1.0f;
  simulationTime = 0.0f;
  setCameraPreset(1);
  WorldConfig defaultConfig;
  setWorldConfig(defaultConfig);
  Debug::log(Debug::Category::MAIN, "Application Resetted");
}

void Application::cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
  auto* const app =
      reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->input.onMouseMove(xpos, ypos);
}

void Application::mouseButtonCallback(GLFWwindow* win, int button, int action,
                                      int mods) {
  auto* const app =
      reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->input.onMouseButton(button, action, mods);
}

void Application::scrollCallback(GLFWwindow* win, double xoffset,
                                 double yoffset) {
  auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(win));
  app->input.onScroll(xoffset, yoffset);
}

void Application::createDefaultPipelineConfig(PipelineConfigInfo& configInfo) {
  configInfo.inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  configInfo.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  configInfo.inputAssembly.primitiveRestartEnable = VK_FALSE;

  configInfo.viewportState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  configInfo.viewportState.viewportCount = 1;
  configInfo.viewportState.scissorCount = 1;

  configInfo.rasterizer.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  configInfo.rasterizer.depthClampEnable = VK_FALSE;
  configInfo.rasterizer.rasterizerDiscardEnable = VK_FALSE;
  configInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  configInfo.rasterizer.lineWidth = 1.0f;
  configInfo.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  configInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  configInfo.rasterizer.depthBiasEnable = VK_FALSE;

  configInfo.multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  configInfo.multisampling.sampleShadingEnable = VK_FALSE;
  configInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  configInfo.depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  configInfo.depthStencil.depthTestEnable = VK_TRUE;
  configInfo.depthStencil.depthWriteEnable = VK_TRUE;
  configInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  configInfo.depthStencil.depthBoundsTestEnable = VK_FALSE;
  configInfo.depthStencil.stencilTestEnable = VK_FALSE;

  configInfo.colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
  configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  configInfo.colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  configInfo.colorBlending.logicOpEnable = VK_FALSE;
  configInfo.colorBlending.attachmentCount = 1;
  configInfo.colorBlending.pAttachments = &configInfo.colorBlendAttachment;

  configInfo.dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                              VK_DYNAMIC_STATE_SCISSOR};
  configInfo.dynamicState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  configInfo.dynamicState.dynamicStateCount =
      static_cast<uint32_t>(configInfo.dynamicStates.size());
  configInfo.dynamicState.pDynamicStates = configInfo.dynamicStates.data();
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

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
      vertShaderStageInfo, fragShaderStageInfo};

  PipelineConfigInfo configInfo{};
  createDefaultPipelineConfig(configInfo);

  // Customize for Particles
  configInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
  configInfo.depthStencil.depthWriteEnable = VK_FALSE;

  configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
  configInfo.colorBlendAttachment.srcColorBlendFactor =
      VK_BLEND_FACTOR_SRC_ALPHA;
  configInfo.colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

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

  configInfo.vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  configInfo.vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindings.size());
  configInfo.vertexInputInfo.pVertexBindingDescriptions = bindings.data();
  configInfo.vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributes.size());
  configInfo.vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

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
  pipelineInfo.pVertexInputState = &configInfo.vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &configInfo.inputAssembly;
  pipelineInfo.pViewportState = &configInfo.viewportState;
  pipelineInfo.pRasterizationState = &configInfo.rasterizer;
  pipelineInfo.pMultisampleState = &configInfo.multisampling;
  pipelineInfo.pDepthStencilState = &configInfo.depthStencil;
  pipelineInfo.pColorBlendState = &configInfo.colorBlending;
  pipelineInfo.pDynamicState = &configInfo.dynamicState;
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

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
      vertShaderStageInfo, fragShaderStageInfo};

  PipelineConfigInfo configInfo{};
  createDefaultPipelineConfig(configInfo);

  // Customize for Shadows
  configInfo.rasterizer.depthBiasEnable = VK_TRUE;
  configInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  configInfo.colorBlending.attachmentCount = 0;
  configInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
  configInfo.dynamicState.dynamicStateCount =
      static_cast<uint32_t>(configInfo.dynamicStates.size());
  configInfo.dynamicState.pDynamicStates = configInfo.dynamicStates.data();

  const auto bindingDescription = Vertex::getBindingDescription();
  const auto attributeDescriptions = Vertex::getAttributeDescriptions();

  configInfo.vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  configInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
  configInfo.vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  configInfo.vertexInputInfo.vertexAttributeDescriptionCount = 1;
  configInfo.vertexInputInfo.pVertexAttributeDescriptions =
      &attributeDescriptions[0];

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
  pipelineInfo.pVertexInputState = &configInfo.vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &configInfo.inputAssembly;
  pipelineInfo.pViewportState = &configInfo.viewportState;
  pipelineInfo.pRasterizationState = &configInfo.rasterizer;
  pipelineInfo.pMultisampleState = &configInfo.multisampling;
  pipelineInfo.pDepthStencilState = &configInfo.depthStencil;
  pipelineInfo.pColorBlendState = &configInfo.colorBlending;
  pipelineInfo.pDynamicState = &configInfo.dynamicState;
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
    const int shadowMapSize = 16384;
    renderingInfo.renderArea = {{0, 0}, {shadowMapSize, shadowMapSize}};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      shadowPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(shadowMapSize);
    viewport.height = static_cast<float>(shadowMapSize);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {shadowMapSize, shadowMapSize};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdSetDepthBias(commandBuffer, 0.0f, 0.0f, 0.0f);

    struct ShadowPushConstants {
      glm::mat4 lightSpaceMatrix;
      glm::mat4 model;
    } shadowPush;

    shadowPush.lightSpaceMatrix = shadowMap.lightSpaceMatrix;

    for (const auto& object : sceneObjects) {
      if (!object.isVisible()) continue;
      if (object.getMeshID() == INVALID_MESH_ID) continue;

      const Mesh* const mesh = meshManager->getMesh(object.getMeshID());
      if (!mesh || mesh->vertexBuffer == VK_NULL_HANDLE) continue;

      shadowPush.model = object.getModelMatrix();

      vkCmdPushConstants(commandBuffer, shadowPipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0,
                         sizeof(ShadowPushConstants), &shadowPush);

      std::array<VkBuffer, 1> vertexBuffers = {mesh->vertexBuffer};
      std::array<VkDeviceSize, 1> offsets = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(),
                             offsets.data());
      vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0,
                           VK_INDEX_TYPE_UINT16);

      vkCmdDrawIndexed(commandBuffer,
                       static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
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

  for (size_t i = 0; i < sceneObjects.size(); ++i) {
    if (plantObjectIndicesSet.count(i) > 0) {
      continue;
    }

    const auto& object = sceneObjects[i];
    if (!object.isVisible()) continue;
    if (object.getMeshID() == INVALID_MESH_ID) continue;
    if (object.getMaterialID() == INVALID_MATERIAL_ID) continue;
    if (object.getName() == "Skybox Sphere") continue;

    const Mesh* const mesh = meshManager->getMesh(object.getMeshID());
    const Material* const material =
        materialManager->getMaterial(object.getMaterialID());

    if (!mesh || mesh->vertexBuffer == VK_NULL_HANDLE) continue;
    if (!material || material->getDescriptorSet() == VK_NULL_HANDLE) continue;

    StandardPushConstants pushConstants;
    pushConstants.model = object.getModelMatrix();
    pushConstants.layerMask = object.getLayerMask();
    pushConstants.cameraLayer = currentCameraLayer;

    vkCmdPushConstants(
        commandBuffer, mainPipeline->getPipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(StandardPushConstants), &pushConstants);

    std::array<VkBuffer, 1> vertexBuffers = {mesh->vertexBuffer};
    std::array<VkDeviceSize, 1> offsets = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(),
                           offsets.data());
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

  renderPlants(commandBuffer, currentFrame);

  const Object* const domeGlassObject = &sceneObjects[2];
  const float timeOfDay = worldState.getTime().normalizedTime;
  const float sunIntensity = worldState.getSunIntensity();
  skybox->render(commandBuffer, descriptorSets[currentFrame], swapChainExtent,
                 domeGlassObject, timeOfDay, sunIntensity);

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
void Application::renderPlants(VkCommandBuffer commandBuffer,
                               uint32_t frameIndex) {
  if (plantObjectIndicesSet.empty()) {
    return;
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    plantPipeline);

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

  const PlantWindData& windData = plantManager->getWindData();

  static float windDebugTimer = 0.0f;
  windDebugTimer += 0.016f;
  if (windDebugTimer >= 5.0f) {
    windDebugTimer = 0.0f;
    Debug::log(Debug::Category::PLANTMANAGER,
               "Rendering plants with wind - Dir: (", windData.windDirection.x,
               ", ", windData.windDirection.y, ", ", windData.windDirection.z,
               ") Strength: ", windData.windStrength, " Time: ", windData.time,
               " SwayAmount: ", windData.swayAmount,
               " SwaySpeed: ", windData.swaySpeed);
  }

  for (size_t const idx : plantObjectIndicesSet) {
    if (idx >= sceneObjects.size()) continue;

    const Object& object = sceneObjects[idx];
    if (!object.isVisible()) continue;
    if (object.getMeshID() == INVALID_MESH_ID) continue;
    if (object.getMaterialID() == INVALID_MATERIAL_ID) continue;

    const Mesh* const mesh = meshManager->getMesh(object.getMeshID());
    const Material* const material =
        materialManager->getMaterial(object.getMaterialID());

    if (!mesh || mesh->vertexBuffer == VK_NULL_HANDLE) continue;
    if (!material || material->getDescriptorSet() == VK_NULL_HANDLE) continue;

    PlantPushConstants pushConstants{};
    pushConstants.model = object.getModelMatrix();
    pushConstants.windDirection = windData.windDirection;
    pushConstants.windStrength = windData.windStrength;
    pushConstants.time = windData.time;
    pushConstants.swayAmount = windData.swayAmount;
    pushConstants.swaySpeed = windData.swaySpeed;
    pushConstants.isPlant = 1.0f;

    vkCmdPushConstants(
        commandBuffer, plantPipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(PlantPushConstants), &pushConstants);

    std::array<VkBuffer, 1> vertexBuffers = {mesh->vertexBuffer};
    std::array<VkDeviceSize, 1> offsets = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(),
                           offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0,
                         VK_INDEX_TYPE_UINT16);

    std::array<VkDescriptorSet, 3> descriptorSetsToBind = {
        descriptorSets[frameIndex], material->getDescriptorSet(),
        lightManager->getShadowDescriptorSet()};

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            plantPipelineLayout, 0,
                            static_cast<uint32_t>(descriptorSetsToBind.size()),
                            descriptorSetsToBind.data(), 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()),
                     1, 0, 0, 0);
  }
}

void Application::createPlantPipeline() {
  Debug::log(Debug::Category::VULKAN, "Creating plant pipeline...");

  const auto vertShaderCode = readFile("SHADERS/plant_vert.spv");
  const auto fragShaderCode = readFile("SHADERS/frag.spv");

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

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
      vertShaderStageInfo, fragShaderStageInfo};

  PipelineConfigInfo configInfo{};
  createDefaultPipelineConfig(configInfo);

  const auto bindingDescription = Vertex::getBindingDescription();
  const auto attributeDescriptions = Vertex::getAttributeDescriptions();

  configInfo.vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  configInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
  configInfo.vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  configInfo.vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  configInfo.vertexInputInfo.pVertexAttributeDescriptions =
      attributeDescriptions.data();

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PlantPushConstants);

  std::array<VkDescriptorSetLayout, 3> layouts = {
      descriptorSetLayout, materialDescriptorSetLayout,
      lightManager->getShadowDescriptorSetLayout()};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &plantPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create plant pipeline layout!");
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
  pipelineInfo.pVertexInputState = &configInfo.vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &configInfo.inputAssembly;
  pipelineInfo.pViewportState = &configInfo.viewportState;
  pipelineInfo.pRasterizationState = &configInfo.rasterizer;
  pipelineInfo.pMultisampleState = &configInfo.multisampling;
  pipelineInfo.pDepthStencilState = &configInfo.depthStencil;
  pipelineInfo.pColorBlendState = &configInfo.colorBlending;
  pipelineInfo.pDynamicState = &configInfo.dynamicState;
  pipelineInfo.layout = plantPipelineLayout;
  pipelineInfo.renderPass = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &plantPipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create plant graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);

  Debug::log(Debug::Category::VULKAN, "Plant pipeline created successfully");
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