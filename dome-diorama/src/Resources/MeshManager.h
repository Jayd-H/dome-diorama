#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Rendering/RenderDevice.h"
#include "Resources/Object.h"

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;
  glm::vec3 normal;

  static VkVertexInputBindingDescription getBindingDescription();
  static std::array<VkVertexInputAttributeDescription, 4>
  getAttributeDescriptions();
};

enum class MeshType { Cube, Sphere, Plane, Cylinder, Custom };

class Mesh final {
 public:
  Mesh() = default;
  ~Mesh() = default;

  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;

  Mesh(Mesh&&) = default;
  Mesh& operator=(Mesh&&) = default;

  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
  MeshType type = MeshType::Custom;
  std::string name;
};

class MeshManager final {
 public:
  explicit MeshManager(RenderDevice* renderDev);
  ~MeshManager();

  MeshManager(const MeshManager&) = delete;
  MeshManager& operator=(const MeshManager&) = delete;

  MeshID createCube(float size = 1.0f);
  MeshID createSphere(float radius = 1.0f, uint32_t segments = 32);
  MeshID createPlane(float width = 1.0f, float height = 1.0f);
  MeshID createCylinder(float radius = 1.0f, float height = 2.0f,
                        uint32_t segments = 32);
  MeshID createParticleQuad();
  MeshID loadFromOBJ(const std::string& filepath);

  Mesh* getMesh(MeshID id);
  const Mesh* getMesh(MeshID id) const;

  MeshID getDefaultCube() const { return defaultCubeID; }

  void cleanup();

  MeshID createProceduralTerrain(float radius, uint32_t segments,
                                 float heightScale = 1.0f,
                                 float noiseScale = 4.0f, int octaves = 4,
                                 float persistence = 0.5f,
                                 unsigned int seed = 0);

 private:
  RenderDevice* renderDevice;
  MeshID defaultCubeID;
  std::vector<std::unique_ptr<Mesh>> meshes;
  std::unordered_map<std::string, MeshID> filepathToID;

  MeshID registerMesh(Mesh* mesh);
  void createBuffers(Mesh* mesh);
  void createDefaultMeshes();
};