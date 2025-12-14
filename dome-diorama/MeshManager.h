#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Object.h"
#include "RenderDevice.h"

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

class Mesh {
 public:
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

  std::string name;
  MeshType type;
};

class MeshManager {
 public:
  MeshManager(RenderDevice* renderDevice);
  ~MeshManager();

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

  std::vector<std::unique_ptr<Mesh>> meshes;
  std::unordered_map<std::string, MeshID> filepathToID;

  MeshID defaultCubeID;

  MeshID registerMesh(Mesh* mesh);
  void createBuffers(Mesh* mesh);
  void createDefaultMeshes();
};