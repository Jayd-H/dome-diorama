#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <unordered_set>
#include <vector>

#include "Particles/ParticleManager.h"
#include "Rendering/MainPipeline.h"
#include "Rendering/PostProcessing.h"
#include "Rendering/PushConstants.h"
#include "Rendering/RenderDevice.h"
#include "Rendering/Window.h"
#include "Resources/MaterialManager.h"
#include "Resources/MeshManager.h"
#include "Resources/Object.h"
#include "Resources/TextureManager.h"
#include "Scene/LightManager.h"
#include "Scene/PlantManager.h"
#include "Scene/PlantState.h"
#include "Scene/Skybox.h"
#include "Scene/WeatherSystem.h"
#include "Scene/WorldState.h"
#include "Util/Camera.h"
#include "Util/Input.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) std::array<glm::mat4, MAX_SHADOW_CASTERS> lightSpaceMatrices;
  alignas(16) glm::vec3 eyePos;
  alignas(4) float time;
};

struct PipelineConfigInfo {
  VkPipelineVertexInputStateCreateInfo vertexInputInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssembly;
  VkPipelineViewportStateCreateInfo viewportState;
  VkPipelineRasterizationStateCreateInfo rasterizer;
  VkPipelineMultisampleStateCreateInfo multisampling;
  VkPipelineDepthStencilStateCreateInfo depthStencil;
  VkPipelineColorBlendStateCreateInfo colorBlending;
  VkPipelineDynamicStateCreateInfo dynamicState;
  std::vector<VkDynamicState> dynamicStates;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
};

class Application final {
 public:
  Application();
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  void init();
  void setScene(const std::vector<Object>& objects);
  void run();

  MeshManager* getMeshManager() const { return meshManager.get(); }
  MaterialManager* getMaterialManager() const { return materialManager.get(); }
  PlantManager* getPlantManager() const { return plantManager.get(); }
  ParticleManager* getParticleManager() const { return particleManager.get(); }
  LightManager* getLightManager() const { return lightManager.get(); }
  WeatherSystem* getWeatherSystem() const { return weatherSystem.get(); }

  void setSunLightID(LightID id) { sunLightID = id; }
  void setWorldConfig(const WorldConfig& config) {
    worldState = WorldState(config);
  }

  void setCameraPreset(int presetIndex);
  void triggerFireEffect();
  void resetApplication();

 private:
  std::vector<Object> sceneObjects;

  std::unique_ptr<Window> window;
  Input input;
  Camera camera;
  WorldState worldState;

  std::unique_ptr<RenderDevice> renderDevice;
  std::unique_ptr<TextureManager> textureManager;
  std::unique_ptr<MaterialManager> materialManager;
  std::unique_ptr<MeshManager> meshManager;
  std::unique_ptr<LightManager> lightManager;
  std::unique_ptr<PlantManager> plantManager;
  std::unique_ptr<ParticleManager> particleManager;
  std::unique_ptr<WeatherSystem> weatherSystem;
  std::unique_ptr<Skybox> skybox;
  std::unique_ptr<PostProcessing> postProcessing;
  std::unique_ptr<MainPipeline> mainPipeline;

  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  std::unordered_set<size_t> plantObjectIndicesSet;

  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout materialDescriptorSetLayout = VK_NULL_HANDLE;

  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

  VkImage depthImage = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
  VkImageView depthImageView = VK_NULL_HANDLE;

  VkPipeline particlePipeline = VK_NULL_HANDLE;
  VkPipelineLayout particlePipelineLayout = VK_NULL_HANDLE;

  VkPipeline shadowPipeline = VK_NULL_HANDLE;
  VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;

  VkPipeline plantPipeline = VK_NULL_HANDLE;
  VkPipelineLayout plantPipelineLayout = VK_NULL_HANDLE;

  VkExtent2D swapChainExtent{0, 0};

  float lastFrameTime = 0.0f;
  float simulationTime = 0.0f;
  float timeScale = 1.0f;
  float scaledDeltaTime = 0.0f;

  VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
  VkFormat depthFormat = VK_FORMAT_UNDEFINED;

  uint32_t currentFrame = 0;

  MeshID particleQuadMesh = 0;
  LightID sunLightID = INVALID_LIGHT_ID;
  LightID moonLightID = INVALID_LIGHT_ID;
  uint32_t currentCameraLayer = 0xFFFFFFFF;

  bool framebufferResized = false;

  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();

  void recreateGraphicsPipeline();
  void createParticlePipeline();
  void createShadowPipeline();
  void createPlantPipeline();
  void createUniformBuffers();
  void drawFrame();
  void recreateSwapChain();
  void cleanupSwapChain();
  void createDepthResources();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void updateUniformBuffer(uint32_t currentImage);

  void renderPlants(VkCommandBuffer commandBuffer, uint32_t frameIndex);

  void recreateTextureSamplers(VkFilter magFilter, VkFilter minFilter);
  void toggleShadingMode();
  void updateCameraLayer();

  static void framebufferResizeCallback(GLFWwindow* window, int width,
                                        int height);
  static void keyCallback(GLFWwindow* window, int key, int scancode, int action,
                          int mods);
  static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void mouseButtonCallback(GLFWwindow* window, int button, int action,
                                  int mods);
  static void scrollCallback(GLFWwindow* window, double xoffset,
                             double yoffset);

  void increaseTemperature();
  void decreaseTemperature();
  void increaseHumidity();
  void decreaseHumidity();
  void increaseWindSpeed();
  void decreaseWindSpeed();
  void cycleWeather();
  void toggleTimePause();

  void createDefaultPipelineConfig(PipelineConfigInfo& configInfo) const;
  void setupRenderingCreateInfo(
      VkPipelineRenderingCreateInfo& createInfo) const;
  void setupViewportScissor(VkCommandBuffer commandBuffer, float width,
                            float height) const;
  void getShaderStages(
      const std::string& vertPath, const std::string& fragPath,
      VkShaderModule& vertModule, VkShaderModule& fragModule,
      std::array<VkPipelineShaderStageCreateInfo, 2>& stages) const;
};