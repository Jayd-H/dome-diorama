//==================================================
// Vulkan ver 1.3 based
//====================================================

// Memory leak detection (everyone say thank you warren)
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Camera.h"
#include "Util/Debug.h"
#include "Particles/FireEmitter.h"
#include "Util/Input.h"
#include "Scene/LightManager.h"
#include "Resources/MaterialManager.h"
#include "Resources/MeshManager.h"
#include "Resources/Object.h"
#include "Particles/ParticleManager.h"
#include "Scene/PlantManager.h"
#include "Rendering/PostProcessing.h"
#include "Rendering/RenderDevice.h"
#include "Resources/TextureManager.h"
#include "Scene/WorldState.h"

#define GLM_FORCE_RADIANS
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

// TODO: implement per-swapchain-image semaphores
#ifdef NDEBUG
const bool enableValidationLayers = false;
const bool DEBUG_MAIN = false;
const bool DEBUG_CAMERA = false;
const bool DEBUG_INPUT = false;
const bool DEBUG_RENDERING = false;
const bool DEBUG_VULKAN = false;
#else
const bool enableValidationLayers = true;
const bool DEBUG_MAIN = true;
const bool DEBUG_CAMERA = false;
const bool DEBUG_INPUT = false;
const bool DEBUG_RENDERING = true;
const bool DEBUG_VULKAN = true;
#endif

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::vec3 eyePos;
};

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

class DomeDiorama final {
 public:
  DomeDiorama() = default;
  ~DomeDiorama() = default;
  DomeDiorama(const DomeDiorama&) = delete;
  DomeDiorama& operator=(const DomeDiorama&) = delete;

  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

 private:
  GLFWwindow* window = nullptr;
  bool framebufferResized = false;
  uint32_t currentFrame = 0;

  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;

  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D swapChainExtent{0, 0};
  std::vector<VkImageView> swapChainImageViews;

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout materialDescriptorSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline graphicsPipeline = VK_NULL_HANDLE;

  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  // Camera & Input
  Input input;
  Camera camera;
  float lastFrameTime = 0.0f;

  TextureManager* textureManager = nullptr;
  MaterialManager* materialManager = nullptr;
  MaterialID testMaterialID = 0;

  // Render Device
  RenderDevice* renderDevice = nullptr;

  // Object stuff
  std::vector<Object> sceneObjects;
  MeshManager* meshManager = nullptr;

  // Depth Buffer
  VkImage depthImage = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
  VkImageView depthImageView = VK_NULL_HANDLE;
  VkFormat depthFormat = VK_FORMAT_UNDEFINED;

  // Callbacks for Inputs!

  static void keyCallback(GLFWwindow* win, int key, int scancode, int action,
                          int mods) {
    if (win == nullptr) return;
    auto* const app =
        reinterpret_cast<DomeDiorama*>(glfwGetWindowUserPointer(win));
    app->input.onKey(key, scancode, action, mods);

    if (action == GLFW_PRESS) {
      if (key == GLFW_KEY_1) {
        app->currentPolygonMode = VK_POLYGON_MODE_FILL;
        app->recreateGraphicsPipeline();
        glfwSetWindowTitle(win, "Dome Diorama - FILL MODE");
        Debug::log(Debug::Category::INPUT, "Switched to FILL mode");
      } else if (key == GLFW_KEY_2) {
        app->currentPolygonMode = VK_POLYGON_MODE_LINE;
        app->recreateGraphicsPipeline();
        glfwSetWindowTitle(win, "Dome Diorama - WIREFRAME MODE");
        Debug::log(Debug::Category::INPUT, "Switched to WIREFRAME mode");
      } else if (key == GLFW_KEY_3) {
        app->currentPolygonMode = VK_POLYGON_MODE_POINT;
        app->recreateGraphicsPipeline();
        glfwSetWindowTitle(win, "Dome Diorama - POINT MODE");
        Debug::log(Debug::Category::INPUT, "Switched to POINT mode");
      } else if (key == GLFW_KEY_4) {
        app->recreateTextureSamplers(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
        glfwSetWindowTitle(win, "Dome Diorama - NEAREST FILTERING");
        Debug::log(Debug::Category::INPUT, "Switched to NEAREST filtering");
      } else if (key == GLFW_KEY_5) {
        app->recreateTextureSamplers(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
        glfwSetWindowTitle(win, "Dome Diorama - LINEAR FILTERING");
        Debug::log(Debug::Category::INPUT, "Switched to LINEAR filtering");
      }
    }
  }

  static void cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
    auto* const app =
        reinterpret_cast<DomeDiorama*>(glfwGetWindowUserPointer(win));
    app->input.onMouseMove(xpos, ypos);
  }

  static void mouseButtonCallback(GLFWwindow* win, int button, int action,
                                  int mods) {
    auto* const app =
        reinterpret_cast<DomeDiorama*>(glfwGetWindowUserPointer(win));
    app->input.onMouseButton(button, action, mods);
  }

  static void scrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
    auto* const app =
        reinterpret_cast<DomeDiorama*>(glfwGetWindowUserPointer(win));
    app->input.onScroll(xoffset, yoffset);
  }

  // Materials and stuff
  MaterialID cactiMaterialID = 0;

  // Lights
  LightManager* lightManager = nullptr;

  // Post Processing
  PostProcessing* postProcessing = nullptr;

  // Particle Stuff
  ParticleManager* particleManager = nullptr;
  VkPipeline particlePipeline = VK_NULL_HANDLE;
  MeshID particleQuadMesh = 0;
  VkPipelineLayout particlePipelineLayout = VK_NULL_HANDLE;

  // Options and stuff
  VkPolygonMode currentPolygonMode = VK_POLYGON_MODE_FILL;

  // World State stuff
  WorldState worldState;
  LightID sunLightID = INVALID_LIGHT_ID;
  LightID moonLightID = INVALID_LIGHT_ID;

  // Shadows!
  VkPipeline shadowPipeline = VK_NULL_HANDLE;
  VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;

  // Plant
  PlantManager* plantManager = nullptr;

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Dome Diorama", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    lastFrameTime = static_cast<float>(glfwGetTime());
  }

  void initVulkan() {
    Debug::log(Debug::Category::VULKAN, "Creating instance...");
    createInstance();
    Debug::log(Debug::Category::VULKAN, "Setting up debug messenger...");
    setupDebugMessenger();
    Debug::log(Debug::Category::VULKAN, "Creating surface...");
    createSurface();
    Debug::log(Debug::Category::VULKAN, "Picking physical device...");
    pickPhysicalDevice();

    Debug::log(Debug::Category::VULKAN, "Physical device: ", physicalDevice);
    if (physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("Physical device is null!");
    }

    Debug::log(Debug::Category::VULKAN, "Creating logical device...");
    createLogicalDevice();
    Debug::log(Debug::Category::VULKAN, "Creating swap chain...");
    createSwapChain();
    Debug::log(Debug::Category::VULKAN, "Creating image views...");
    createImageViews();

    Debug::log(Debug::Category::VULKAN, "Finding depth format...");
    depthFormat = findDepthFormat();
    Debug::log(Debug::Category::VULKAN, "Depth format: ", depthFormat);

    Debug::log(Debug::Category::VULKAN, "Creating descriptor set layout...");
    createDescriptorSetLayout();
    Debug::log(Debug::Category::VULKAN,
               "Creating material descriptor set layout...");
    createMaterialDescriptorSetLayout();

    Debug::log(Debug::Category::VULKAN, "Creating command pool...");
    createCommandPool();

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
    createDescriptorPool();
    Debug::log(Debug::Category::VULKAN, "Initializing material manager...");
    materialManager->init(materialDescriptorSetLayout, descriptorPool);
    Debug::log(Debug::Category::VULKAN, "Initializing light manager...");
    lightManager->init();
    Debug::log(Debug::Category::VULKAN, "Initializing plant manager...");
    plantManager->init();

    Debug::log(Debug::Category::VULKAN, "Creating particle manager...");
    particleManager = new ParticleManager(renderDevice, materialManager);
    Debug::log(Debug::Category::VULKAN, "Initializing particle manager...");
    particleManager->init(materialDescriptorSetLayout, pipelineLayout);

    Debug::log(Debug::Category::VULKAN, "Creating graphics pipeline...");
    createGraphicsPipeline();
    Debug::log(Debug::Category::VULKAN, "Creating particle pipeline...");
    createParticlePipeline();
    Debug::log(Debug::Category::VULKAN, "Creating shadow pipeline...");
    createShadowPipeline();

    Debug::log(Debug::Category::VULKAN, "Creating post-processing...");
    postProcessing =
        new PostProcessing(renderDevice, device, swapChainImageFormat);
    postProcessing->init(descriptorPool, swapChainExtent.width,
                         swapChainExtent.height);

    createScene();

    Debug::log(Debug::Category::VULKAN, "Creating uniform buffers...");
    createUniformBuffers();
    Debug::log(Debug::Category::VULKAN, "Creating descriptor sets...");
    createDescriptorSets();
    Debug::log(Debug::Category::VULKAN, "Creating command buffers...");
    createCommandBuffers();
    Debug::log(Debug::Category::VULKAN, "Creating sync objects...");
    createSyncObjects();
    Debug::log(Debug::Category::VULKAN, "Vulkan initialization complete!");
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      const float currentTime = static_cast<float>(glfwGetTime());
      const float deltaTime = currentTime - lastFrameTime;
      lastFrameTime = currentTime;

      input.update();
      camera.update(input, deltaTime);
      camera.setCursorMode(window);
      input.endFrame();

      drawFrame();
    }
  }
  void cleanup() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

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

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, materialDescriptorSetLayout, nullptr);

    vkDestroyPipeline(device, particlePipeline, nullptr);
    vkDestroyPipelineLayout(device, particlePipelineLayout, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);

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

    if (enableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void setupDebugMessenger() {
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                     &debugMessenger) != VK_SUCCESS) {
      throw std::runtime_error("Failed to set up debug messenger!");
    }
  }

  void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create window surface!");
    }
  }

  void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
      throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
      if (isDeviceSuitable(device)) {
        physicalDevice = device;
        break;
      }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("Failed to find a suitable GPU!");
    }
  }

  void createLogicalDevice() {
    const QueueFamilyIndices queueFamilyIndicesResult =
        findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndicesResult.graphicsFamily.value(),
        queueFamilyIndicesResult.presentFamily.value()};

    float queuePriority = 1.0f;
    for (const uint32_t queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
    dynamicRenderingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features sync2Features{};
    sync2Features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Features.synchronization2 = VK_TRUE;
    dynamicRenderingFeatures.pNext = &sync2Features;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features = deviceFeatures;
    deviceFeatures2.pNext = &dynamicRenderingFeatures;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &deviceFeatures2;
    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr;
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
      createInfo.enabledLayerCount =
          static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
      createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(device, queueFamilyIndicesResult.graphicsFamily.value(), 0,
                     &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilyIndicesResult.presentFamily.value(), 0,
                     &presentQueue);
  }

  void createSwapChain() {
    const SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(physicalDevice);
    const VkSurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR presentMode =
        chooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const QueueFamilyIndices queueIndices = findQueueFamilies(physicalDevice);

    if (queueIndices.graphicsFamily != queueIndices.presentFamily) {
      const std::array<uint32_t, 2> queueFamilyIndicesArr = {
          queueIndices.graphicsFamily.value(),
          queueIndices.presentFamily.value()};
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndicesArr.data();
    } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                            swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
  }

  void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
      VkImageViewCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = swapChainImages[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = swapChainImageFormat;
      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      if (vkCreateImageView(device, &createInfo, nullptr,
                            &swapChainImageViews[i]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image views!");
      }
    }
  }

  void createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding lightLayoutBinding{};
    lightLayoutBinding.binding = 1;
    lightLayoutBinding.descriptorCount = 1;
    lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                            lightLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor set layout!");
    }
  }

  void createGraphicsPipeline() {
    const auto vertShaderCode = readFile("shaders/vert.spv");
    const auto fragShaderCode = readFile("shaders/frag.spv");

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
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = currentPolygonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

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
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    std::array<VkDescriptorSetLayout, 3> layouts = {
        descriptorSetLayout, materialDescriptorSetLayout,
        lightManager->getShadowDescriptorSetLayout()};

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create pipeline layout!");
    }

    VkPipelineRenderingCreateInfo renderingCreateInfo{};
    renderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
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
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
  }

  void recreateGraphicsPipeline() {
    vkDeviceWaitIdle(device);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    createGraphicsPipeline();
  }

  void recreateTextureSamplers(VkFilter magFilter, VkFilter minFilter) {
    vkDeviceWaitIdle(device);

    textureManager->recreateSamplers(magFilter, minFilter);

    Debug::log(Debug::Category::RENDERING,
               "Recreated texture samplers with filter mode: ",
               magFilter == VK_FILTER_NEAREST ? "NEAREST" : "LINEAR");
  }

  void createParticlePipeline() {
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
    attributes[3] = {3, 0, VK_FORMAT_R32G32B32_SFLOAT,
                     offsetof(Vertex, normal)};
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
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
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
    renderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
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

    Debug::log(Debug::Category::VULKAN,
               "Particle pipeline created successfully");
  }

  void createShadowPipeline() {
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
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
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
    renderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
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

  void createCommandPool() {
    const QueueFamilyIndices queueFamilyIndicesResult =
        findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndicesResult.graphicsFamily.value();
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create command pool!");
    }
  }

  void createUniformBuffers() {
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

  void createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount =
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2 + 100);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount =
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 20 + 700);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + 100);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor pool!");
    }
  }

  void createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = uniformBuffers[i];
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(UniformBufferObject);

      VkDescriptorBufferInfo lightBufferInfo{};
      lightBufferInfo.buffer = lightManager->getLightBuffer();
      lightBufferInfo.offset = 0;
      lightBufferInfo.range = sizeof(LightBufferObject);

      std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = descriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &bufferInfo;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = descriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo = &lightBufferInfo;

      vkUpdateDescriptorSets(device,
                             static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
    }
  }

  void createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate command buffers!");
    }
  }

  void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                            &imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                            &renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
              VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create synchronization objects for a frame!");
      }
    }
  }

  void drawFrame() {
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

    result = vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                           inFlightFences[currentFrame]);
    if (result != VK_SUCCESS) {
      std::cerr << "vkQueueSubmit failed with error code: " << result
                << std::endl;
      switch (result) {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
          std::cerr << "  VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
          break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
          std::cerr << "  VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
          break;
        case VK_ERROR_DEVICE_LOST:
          std::cerr << "  VK_ERROR_DEVICE_LOST" << std::endl;
          break;
        default:
          std::cerr << "  Unknown error" << std::endl;
          break;
      }
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

  void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();

    postProcessing->resize(swapChainExtent.width, swapChainExtent.height,
                           descriptorPool);
  }

  void cleanupSwapChain() {
    for (const auto imageView : swapChainImageViews) {
      vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
  }

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("Failed to begin recording command buffer!");
    }

    const auto& shadowMaps = lightManager->getShadowMaps();

    for (size_t smIdx = 0; smIdx < shadowMaps.size(); smIdx++) {
      const auto& shadowMap = shadowMaps[smIdx];
      Light* const light = shadowMap.light;

      VkImageMemoryBarrier2 shadowBarrier{};
      shadowBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
      shadowBarrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
      shadowBarrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
      shadowBarrier.dstStageMask =
          VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
      shadowBarrier.dstAccessMask =
          VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      shadowBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      shadowBarrier.newLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      shadowBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      shadowBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      shadowBarrier.image = shadowMap.image;
      shadowBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      shadowBarrier.subresourceRange.baseMipLevel = 0;
      shadowBarrier.subresourceRange.levelCount = 1;
      shadowBarrier.subresourceRange.baseArrayLayer = 0;
      shadowBarrier.subresourceRange.layerCount = 1;

      VkDependencyInfo depInfo{};
      depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
      depInfo.imageMemoryBarrierCount = 1;
      depInfo.pImageMemoryBarriers = &shadowBarrier;

      vkCmdPipelineBarrier2(commandBuffer, &depInfo);

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
      renderingInfo.pColorAttachments = nullptr;
      renderingInfo.pDepthAttachment = &depthAttachment;
      renderingInfo.pStencilAttachment = nullptr;

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

      vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

      struct ShadowPushConstants {
        glm::mat4 lightSpaceMatrix;
        glm::mat4 model;
      } shadowPush;

      shadowPush.lightSpaceMatrix = light->lightSpaceMatrix;

      for (const auto& object : sceneObjects) {
        if (!object.visible) continue;
        if (object.meshID == INVALID_MESH_ID) continue;

        const Mesh* const mesh = meshManager->getMesh(object.meshID);
        if (!mesh || mesh->vertexBuffer == VK_NULL_HANDLE) continue;

        shadowPush.model = object.getModelMatrix();

        vkCmdPushConstants(commandBuffer, shadowPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(ShadowPushConstants), &shadowPush);

        const std::array<VkBuffer, 1> vertexBuffersArr = {mesh->vertexBuffer};
        const std::array<VkDeviceSize, 1> offsetsArr = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffersArr.data(),
                               offsetsArr.data());
        vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0,
                             VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0,
                         0);
      }

      vkCmdEndRendering(commandBuffer);

      shadowBarrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
      shadowBarrier.srcAccessMask =
          VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      shadowBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
      shadowBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
      shadowBarrier.oldLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      shadowBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

      vkCmdPipelineBarrier2(commandBuffer, &depInfo);
    }

    postProcessing->beginOffscreenPass(commandBuffer, depthImageView,
                                       swapChainExtent);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);

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

      const Mesh* const mesh = meshManager->getMesh(object.meshID);
      const Material* const material =
          materialManager->getMaterial(object.materialID);

      if (!mesh || mesh->vertexBuffer == VK_NULL_HANDLE) continue;
      if (!material || material->descriptorSet == VK_NULL_HANDLE) continue;

      const glm::mat4 modelMatrix = object.getModelMatrix();
      vkCmdPushConstants(commandBuffer, pipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                         &modelMatrix);

      const std::array<VkBuffer, 1> vertexBuffersArr = {mesh->vertexBuffer};
      const std::array<VkDeviceSize, 1> offsetsArr = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffersArr.data(),
                             offsetsArr.data());
      vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0,
                           VK_INDEX_TYPE_UINT16);

      std::array<VkDescriptorSet, 3> descriptorSetsToBind = {
          descriptorSets[currentFrame], material->descriptorSet,
          lightManager->getShadowDescriptorSet()};

      vkCmdBindDescriptorSets(
          commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
          static_cast<uint32_t>(descriptorSetsToBind.size()),
          descriptorSetsToBind.data(), 0, nullptr);

      vkCmdDrawIndexed(commandBuffer,
                       static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
    }

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

 void updateUniformBuffer(uint32_t currentImage) {
    static const auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time =
        std::chrono::duration<float>(currentTime - startTime).count();

    const float deltaTime = time - (time - 0.016f);

    worldState.update(deltaTime);
    particleManager->update(deltaTime);

    const float sunOrbitRadius = 150.0f;
    const glm::vec3 sunDirection = worldState.getSunDirection();
    const glm::vec3 sunPosition = sunDirection * sunOrbitRadius;
    sceneObjects[0].setPosition(sunPosition);

    const float moonOrbitRadius = 130.0f;
    const glm::vec3 moonDirection = worldState.getMoonDirection();
    const glm::vec3 moonPosition = moonDirection * moonOrbitRadius;
    sceneObjects[1].setPosition(moonPosition);

    Light* const sunLight = lightManager->getLight(sunLightID);
    if (sunLight) {
      sunLight->direction = glm::normalize(-sunDirection);
      const float sunHeight = sunDirection.y;
      float intensity = 0.0f;

      if (sunHeight > 0.0f) {
        intensity = glm::smoothstep(0.0f, 0.3f, sunHeight) * 8.0f;
      }

      sunLight->intensity = intensity * worldState.getSunIntensity();

      glm::vec3 sunColor;
      if (sunHeight < 0.0f) {
        sunColor = glm::vec3(0.0f, 0.0f, 0.0f);
      } else if (sunHeight < 0.2f) {
        const float t = sunHeight / 0.2f;
        const glm::vec3 sunriseColor = glm::vec3(1.0f, 0.4f, 0.1f);
        const glm::vec3 dayColor = glm::vec3(1.0f, 0.95f, 0.85f);
        sunColor = glm::mix(sunriseColor, dayColor, t);
      } else if (sunHeight > 0.8f) {
        sunColor = glm::vec3(1.0f, 1.0f, 0.95f);
      } else {
        const float t = (sunHeight - 0.2f) / 0.6f;
        const glm::vec3 dayColor = glm::vec3(1.0f, 0.95f, 0.85f);
        const glm::vec3 noonColor = glm::vec3(1.0f, 1.0f, 0.95f);
        sunColor = glm::mix(dayColor, noonColor, t);
      }

      sunLight->color = sunColor;

      const glm::vec3 lightEye = sunPosition;
      const glm::vec3 lightCenter = glm::vec3(0.0f, 0.0f, 0.0f);
      const glm::vec3 lightUp = glm::vec3(0.0f, 1.0f, 0.0f);

      const glm::mat4 lightView = glm::lookAt(lightEye, lightCenter, lightUp);
      const glm::mat4 lightProjection =
          glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, 1.0f, 500.0f);
      const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

      lightManager->updateLightSpaceMatrix(sunLightID, lightSpaceMatrix);
    }

    Light* const moonLight = lightManager->getLight(moonLightID);
    if (moonLight) {
      moonLight->direction = glm::normalize(-moonDirection);
      const float moonHeight = moonDirection.y;
      float intensity = 0.0f;

      if (moonHeight > 0.0f) {
        intensity = glm::smoothstep(0.0f, 0.3f, moonHeight) * 2.0f;
      }

      moonLight->intensity = intensity * worldState.getMoonIntensity();

      glm::vec3 moonColor;
      if (moonHeight < 0.0f) {
        moonColor = glm::vec3(0.0f, 0.0f, 0.0f);
      } else if (moonHeight < 0.15f) {
        const float t = moonHeight / 0.15f;
        const glm::vec3 horizonColor = glm::vec3(0.3f, 0.35f, 0.5f);
        const glm::vec3 nightColor = glm::vec3(0.4f, 0.5f, 0.7f);
        moonColor = glm::mix(horizonColor, nightColor, t);
      } else {
        moonColor = glm::vec3(0.4f, 0.5f, 0.7f);
      }

      moonLight->color = moonColor;

      const glm::vec3 lightEye = moonPosition;
      const glm::vec3 lightCenter = glm::vec3(0.0f, 0.0f, 0.0f);
      const glm::vec3 lightUp = glm::vec3(0.0f, 1.0f, 0.0f);

      const glm::mat4 lightView = glm::lookAt(lightEye, lightCenter, lightUp);
      const glm::mat4 lightProjection =
          glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, 1.0f, 500.0f);
      const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

      lightManager->updateLightSpaceMatrix(moonLightID, lightSpaceMatrix);
    }

    lightManager->updateLightBuffer();

    UniformBufferObject ubo{};
    ubo.view = camera.getViewMatrix();
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        swapChainExtent.width / static_cast<float>(swapChainExtent.height),
        0.1f, 50000.0f);
    ubo.proj[1][1] *= -1;

    glm::vec3 camPos = glm::vec3(0.0f);
    if (camera.getMode() == CameraMode::ORBIT) {
      const float camX = 35.0f * sin(0.5f) * cos(0.0f);
      const float camY = 35.0f * cos(0.5f);
      const float camZ = 35.0f * sin(0.5f) * sin(0.0f);
      camPos = glm::vec3(camX, camY, camZ);
    }
    ubo.eyePos = camPos;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
  }

  void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT& createInfo) const {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
  }

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features) {
    Debug::log(Debug::Category::VULKAN,
               "In findSupportedFormat, this = ", this);
    Debug::log(Debug::Category::VULKAN, "physicalDevice = ", physicalDevice);
    Debug::log(Debug::Category::VULKAN,
               "candidates.size() = ", candidates.size());

    for (VkFormat format : candidates) {
      Debug::log(Debug::Category::VULKAN, "Checking format: ", format);
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR &&
          (props.linearTilingFeatures & features) == features) {
        Debug::log(Debug::Category::VULKAN,
                   "Found suitable format (linear): ", format);
        return format;
      } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                 (props.optimalTilingFeatures & features) == features) {
        Debug::log(Debug::Category::VULKAN,
                   "Found suitable format (optimal): ", format);
        return format;
      }
    }

    throw std::runtime_error("failed to find supported format!");
  }

  VkFormat findDepthFormat() {
    Debug::log(Debug::Category::VULKAN, "In findDepthFormat, this = ", this);
    Debug::log(Debug::Category::VULKAN, "physicalDevice = ", physicalDevice);

    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  void createDepthResources() {
    Debug::log(Debug::Category::VULKAN, "Creating depth image...");
    Debug::log(Debug::Category::VULKAN, "Depth format: ", depthFormat);
    Debug::log(Debug::Category::VULKAN, "Extent: ", swapChainExtent.width, "x",
               swapChainExtent.height);

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
      throw std::runtime_error("failed to create depth image!");
    }
    Debug::log(Debug::Category::VULKAN, "Depth image created successfully");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, depthImage, &memRequirements);
    Debug::log(Debug::Category::VULKAN,
               "Memory requirements size: ", memRequirements.size);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = renderDevice->findMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to allocate depth image memory!");
    }
    Debug::log(Debug::Category::VULKAN, "Depth image memory allocated");

    vkBindImageMemory(device, depthImage, depthImageMemory, 0);
    Debug::log(Debug::Category::VULKAN, "Depth image memory bound");

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
      throw std::runtime_error("failed to create depth image view!");
    }
    Debug::log(Debug::Category::VULKAN,
               "Depth image view created successfully");
  }

  bool isDeviceSuitable(VkPhysicalDevice dev) const {
    const QueueFamilyIndices queueIndices = findQueueFamilies(dev);
    const bool extensionsSupported = checkDeviceExtensionSupport(dev);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
      const SwapChainSupportDetails swapChainSupport =
          querySwapChainSupport(dev);
      swapChainAdequate = !swapChainSupport.formats.empty() &&
                          !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
    dynamicRenderingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &dynamicRenderingFeatures;
    vkGetPhysicalDeviceFeatures2(dev, &features2);

    return queueIndices.isComplete() && extensionsSupported &&
           swapChainAdequate && dynamicRenderingFeatures.dynamicRendering;
  }

  bool checkDeviceExtensionSupport(VkPhysicalDevice dev) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount,
                                         nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount,
                                         availableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                             deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
  }

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev) const {
    QueueFamilyIndices queueIndices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount,
                                             queueFamilies.data());
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        queueIndices.graphicsFamily = i;
      }
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
      if (presentSupport) {
        queueIndices.presentFamily = i;
      }
      if (queueIndices.isComplete()) {
        break;
      }
      i++;
    }
    return queueIndices;
  }

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev) const {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface,
                                              &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
    if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount,
                                           details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount,
                                              nullptr);
    if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount,
                                                details.presentModes.data());
    }
    return details;
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
    for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
      }
    }
    return availableFormats[0];
  }

  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes) const {
    for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return availablePresentMode;
      }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(
      const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    } else {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                 static_cast<uint32_t>(height)};
      actualExtent.width =
          std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                     capabilities.maxImageExtent.width);
      actualExtent.height =
          std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                     capabilities.maxImageExtent.height);
      return actualExtent;
    }
  }

  std::vector<const char*> getRequiredExtensions() const {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions,
                                        glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
  }

  bool checkValidationLayerSupport() const {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* const layerName : validationLayers) {
      bool layerFound = false;
      for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
          layerFound = true;
          break;
        }
      }
      if (!layerFound) {
        return false;
      }
    }
    return true;
  }

  static std::vector<char> readFile(const std::string& filename) {
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

  void createInstance() {
    if (enableValidationLayers) {
      if (!checkValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
      }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Dome Diorama";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    const auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
      createInfo.enabledLayerCount =
          static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();

      VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
      populateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = &debugCreateInfo;

      if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
      }
    } else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;

      if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
      }
    }
  }

  VkShaderModule createShaderModule(const std::vector<char>& code) const {
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

  static void framebufferResizeCallback(GLFWwindow* win, int width,
                                        int height) {
    auto* const app =
        reinterpret_cast<DomeDiorama*>(glfwGetWindowUserPointer(win));
    app->framebufferResized = true;
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* /*pUserData*/) {
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  void createMaterialDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 8> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    for (uint32_t i = 1; i < 8; i++) {
      bindings[i].binding = i;
      bindings[i].descriptorCount = 1;
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                    &materialDescriptorSetLayout) !=
        VK_SUCCESS) {
      throw std::runtime_error(
          "Failed to create material descriptor set layout!");
    }
  }

 void createScene() {
    Debug::log(Debug::Category::MAIN, "Creating materials and scene...");

    const MaterialID sunMaterialID =
        materialManager->registerMaterial(MaterialBuilder()
                                              .name("Sun Material")
                                              .albedoColor(1.0f, 0.9f, 0.6f)
                                              .emissiveIntensity(1.0f)
                                              .roughness(1.0f)
                                              .metallic(0.0f));

    const MaterialID moonMaterialID =
        materialManager->registerMaterial(MaterialBuilder()
                                              .name("Moon Material")
                                              .albedoColor(0.7f, 0.7f, 0.8f)
                                              .emissiveIntensity(0.3f)
                                              .roughness(0.8f)
                                              .metallic(0.0f));

    const MaterialID sandMaterialID = materialManager->registerMaterial(
        MaterialBuilder()
            .name("Sand Material")
            .albedoMap("./Models/textures/gravelly_sand_diff_1k.jpg")
            .normalMap("./Models/textures/gravelly_sand_nor_gl_1k.png")
            .roughnessMap("./Models/textures/gravelly_sand_rough_1k.png")
            .heightMap("./Models/textures/gravelly_sand_disp_1k.png")
            .heightScale(0.02f)
            .textureScale(40.0f));

    const MeshID sphereMesh = meshManager->createSphere(10.0f, 32);
    const MeshID sandTerrainMesh = meshManager->createProceduralTerrain(
        100.0f, 100, 10.0f, 2.0f, 2, 0.6f, 42);

    const Object sun = ObjectBuilder()
                           .name("Sun")
                           .position(0.0f, 0.0f, 0.0f)
                           .mesh(sphereMesh)
                           .material(sunMaterialID)
                           .scale(1.0f)
                           .build();

    const Object moon = ObjectBuilder()
                            .name("Moon")
                            .position(0.0f, 0.0f, 0.0f)
                            .mesh(sphereMesh)
                            .material(moonMaterialID)
                            .scale(0.8f)
                            .build();

    const Object sandPlane = ObjectBuilder()
                                 .name("Sand Terrain")
                                 .position(0.0f, 0.0f, 0.0f)
                                 .mesh(sandTerrainMesh)
                                 .material(sandMaterialID)
                                 .build();

    sceneObjects.push_back(sun);
    sceneObjects.push_back(moon);
    sceneObjects.push_back(sandPlane);

    const Mesh* const terrainMesh = meshManager->getMesh(sandTerrainMesh);

    PlantSpawnConfig plantConfig;
    plantConfig.numCacti = 150;
    plantConfig.numTrees = 100;
    plantConfig.minRadius = 10.0f;
    plantConfig.maxRadius = 90.0f;
    plantConfig.seed = 42;
    plantConfig.randomGrowthStages = true;
    plantConfig.scaleVariance = 0.3f;
    plantConfig.rotationVariance = 0.2f;

    plantManager->spawnPlantsOnTerrain(sceneObjects, terrainMesh, plantConfig);

    const Light sunLight = LightBuilder()
                               .type(LightType::Directional)
                               .name("Sun Light")
                               .direction(0.0f, -1.0f, 0.0f)
                               .color(1.0f, 0.95f, 0.8f)
                               .intensity(5.0f)
                               .castsShadows(true)
                               .build();

    const Light moonLight = LightBuilder()
                                .type(LightType::Directional)
                                .name("Moon Light")
                                .direction(0.0f, -1.0f, 0.0f)
                                .color(0.6f, 0.7f, 1.0f)
                                .intensity(0.3f)
                                .castsShadows(true)
                                .build();

    const Light accentLight = LightBuilder()
                                  .type(LightType::Point)
                                  .name("Accent Light")
                                  .position(-300.0f, 200.0f, 300.0f)
                                  .color(1.0f, 0.8f, 0.6f)
                                  .intensity(2000.0f)
                                  .build();

    sunLightID = lightManager->addLight(sunLight);
    moonLightID = lightManager->addLight(moonLight);
    static_cast<void>(lightManager->addLight(accentLight));

    const MaterialID particleMaterialID =
        materialManager->registerMaterial(MaterialBuilder()
                                              .name("Particle Material")
                                              .albedoColor(1.0f, 1.0f, 1.0f)
                                              .roughness(0.0f)
                                              .metallic(0.0f)
                                              .transparent(true));

    FireEmitter* const fireEmitter = FireEmitterBuilder()
                                         .name("Fire Emitter")
                                         .position(0.0f, 0.5f, 0.0f)
                                         .maxParticles(500)
                                         .particleLifetime(2.0f)
                                         .material(particleMaterialID)
                                         .waveFrequency(2.0f)
                                         .waveAmplitude(0.5f)
                                         .baseColor(1.0f, 0.9f, 0.1f)
                                         .tipColor(1.0f, 0.3f, 0.0f)
                                         .upwardSpeed(2.0f)
                                         .spawnRadius(0.2f)
                                         .particleScale(0.5f)
                                         .build();

    particleManager->registerEmitter(fireEmitter);

    Debug::log(Debug::Category::MAIN, "Created particle emitter");
    Debug::log(Debug::Category::MAIN, "Created ", sceneObjects.size(),
               " scene objects and ", lightManager->getLightCount(), " lights");
    Debug::log(Debug::Category::MAIN, "Spawned ",
               plantManager->getPlants().size(), " plants");
  }
};

int main() {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  Debug::setEnabled(Debug::Category::MAIN, DEBUG_MAIN);
  Debug::setEnabled(Debug::Category::CAMERA, DEBUG_CAMERA);
  Debug::setEnabled(Debug::Category::INPUT, DEBUG_INPUT);
  Debug::setEnabled(Debug::Category::RENDERING, DEBUG_RENDERING);
  Debug::setEnabled(Debug::Category::VULKAN, DEBUG_VULKAN);

  try {
    DomeDiorama application;
    application.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}