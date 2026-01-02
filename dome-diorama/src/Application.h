#pragma once
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "Particles/ParticleManager.h"
#include "Rendering/MainPipeline.h"
#include "Rendering/PostProcessing.h"
#include "Rendering/RenderDevice.h"
#include "Rendering/Window.h"
#include "Resources/MaterialManager.h"
#include "Resources/MeshManager.h"
#include "Resources/Object.h"
#include "Resources/TextureManager.h"
#include "Scene/LightManager.h"
#include "Scene/PlantManager.h"
#include "Scene/Skybox.h"
#include "Scene/WorldState.h"
#include "Util/Camera.h"
#include "Util/Input.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::vec3 eyePos;
  alignas(4) float time;
  alignas(16) glm::mat4 lightSpaceMatrices[MAX_SHADOW_CASTERS];
};

class Application final {
 public:
  Application();
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  void init();
  void setScene(const std::vector<Object>& objects) { sceneObjects = objects; }
  void run();

  MeshManager* getMeshManager() const { return meshManager; }
  MaterialManager* getMaterialManager() const { return materialManager; }
  PlantManager* getPlantManager() const { return plantManager; }
  ParticleManager* getParticleManager() const { return particleManager; }
  LightManager* getLightManager() const { return lightManager; }

  void setSunLightID(LightID id) { sunLightID = id; }

  void setWorldConfig(const WorldConfig& config) {
    worldState = WorldState(config);
  }

 private:
  std::unique_ptr<Window> window;
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

  Input input;
  Camera camera;
  float lastFrameTime = 0.0f;

  TextureManager* textureManager = nullptr;
  MaterialManager* materialManager = nullptr;
  RenderDevice* renderDevice = nullptr;
  MeshManager* meshManager = nullptr;
  LightManager* lightManager = nullptr;
  PlantManager* plantManager = nullptr;
  PostProcessing* postProcessing = nullptr;
  ParticleManager* particleManager = nullptr;
  Skybox* skybox = nullptr;

  std::vector<Object> sceneObjects;

  VkImage depthImage = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
  VkImageView depthImageView = VK_NULL_HANDLE;
  VkFormat depthFormat = VK_FORMAT_UNDEFINED;

  VkPipeline particlePipeline = VK_NULL_HANDLE;
  MeshID particleQuadMesh = 0;
  VkPipelineLayout particlePipelineLayout = VK_NULL_HANDLE;

  WorldState worldState;
  LightID sunLightID = INVALID_LIGHT_ID;
  LightID moonLightID = INVALID_LIGHT_ID;

  VkPipeline shadowPipeline = VK_NULL_HANDLE;
  VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;

  std::unique_ptr<MainPipeline> mainPipeline;

  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();

  void recreateGraphicsPipeline();
  void createParticlePipeline();
  void createShadowPipeline();
  void createUniformBuffers();
  void drawFrame();
  void recreateSwapChain();
  void cleanupSwapChain();
  void createDepthResources();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void updateUniformBuffer(uint32_t currentImage);

  void recreateTextureSamplers(VkFilter magFilter, VkFilter minFilter);
  void toggleShadingMode();

  static void framebufferResizeCallback(GLFWwindow* window, int width,
                                        int height);
  static void keyCallback(GLFWwindow* window, int key, int scancode, int action,
                          int mods);
  static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void mouseButtonCallback(GLFWwindow* window, int button, int action,
                                  int mods);
  static void scrollCallback(GLFWwindow* window, double xoffset,
                             double yoffset);

  VkShaderModule createShaderModule(const std::vector<char>& code) const;
  static std::vector<char> readFile(const std::string& filename);
};