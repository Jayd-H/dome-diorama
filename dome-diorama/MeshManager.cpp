#include "MeshManager.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include "Debug.h"
#include "PerlinNoise.h"

VkVertexInputBindingDescription Vertex::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4>
Vertex::getAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

  attributeDescriptions[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                              offsetof(Vertex, pos)};
  attributeDescriptions[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT,
                              offsetof(Vertex, color)};
  attributeDescriptions[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT,
                              offsetof(Vertex, texCoord)};
  attributeDescriptions[3] = {3, 0, VK_FORMAT_R32G32B32_SFLOAT,
                              offsetof(Vertex, normal)};

  return attributeDescriptions;
}

MeshManager::MeshManager(RenderDevice* renderDevice)
    : renderDevice(renderDevice), defaultCubeID(0) {
  Debug::log(Debug::Category::RENDERING, "MeshManager: Constructor called");

  meshes.push_back(std::unique_ptr<Mesh>(nullptr));

  createDefaultMeshes();

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Initialization complete");
}

MeshManager::~MeshManager() {
  Debug::log(Debug::Category::RENDERING, "MeshManager: Destructor called");
  cleanup();
}

MeshID MeshManager::createCube(float size) {
  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Creating cube (size: ", size, ")");

  Mesh* mesh = new Mesh();
  mesh->name = "Cube";
  mesh->type = MeshType::Cube;

  float h = size * 0.5f;

  mesh->vertices = {
      {{-h, -h, -h}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
      {{h, -h, -h}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
      {{h, h, -h}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
      {{-h, h, -h}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},

      {{h, -h, -h}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
      {{h, -h, h}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
      {{h, h, h}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{h, h, -h}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},

      {{h, -h, h}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      {{-h, -h, h}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      {{-h, h, h}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{h, h, h}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},

      {{-h, -h, h}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
      {{-h, -h, -h}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
      {{-h, h, -h}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
      {{-h, h, h}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},

      {{-h, h, -h}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
      {{h, h, -h}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
      {{h, h, h}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{-h, h, h}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

      {{-h, -h, h}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
      {{h, -h, h}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
      {{h, -h, -h}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
      {{-h, -h, -h}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}}};

  mesh->indices = {0,  2,  1,  0,  3,  2,  4,  6,  5,  4,  7,  6,
                   8,  10, 9,  8,  11, 10, 12, 14, 13, 12, 15, 14,
                   16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22};

  createBuffers(mesh);

  MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Created cube with ID: ", id);

  return id;
}

MeshID MeshManager::createSphere(float radius, uint32_t segments) {
  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Creating sphere (radius: ", radius,
             ", segments: ", segments, ")");

  Mesh* mesh = new Mesh();
  mesh->name = "Sphere";
  mesh->type = MeshType::Sphere;

  uint32_t sliceCount = segments;
  uint32_t stackCount = segments;

  Vertex topVertex;
  topVertex.pos = glm::vec3(0.0f, radius, 0.0f);
  topVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  topVertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
  topVertex.texCoord = glm::vec2(0.0f, 0.0f);
  mesh->vertices.push_back(topVertex);

  float phiStep = glm::pi<float>() / stackCount;
  float thetaStep = 2.0f * glm::pi<float>() / sliceCount;

  for (uint32_t i = 1; i <= stackCount - 1; ++i) {
    float phi = i * phiStep;
    for (uint32_t j = 0; j <= sliceCount; ++j) {
      float theta = j * thetaStep;

      Vertex v;
      v.pos.x = radius * sinf(phi) * cosf(theta);
      v.pos.y = radius * cosf(phi);
      v.pos.z = radius * sinf(phi) * sinf(theta);
      v.color = glm::vec3(1.0f, 1.0f, 1.0f);
      v.normal = glm::normalize(v.pos);
      v.texCoord = glm::vec2(static_cast<float>(j) / sliceCount,
                             static_cast<float>(i) / stackCount);

      mesh->vertices.push_back(v);
    }
  }

  Vertex bottomVertex;
  bottomVertex.pos = glm::vec3(0.0f, -radius, 0.0f);
  bottomVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  bottomVertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
  bottomVertex.texCoord = glm::vec2(0.0f, 1.0f);
  mesh->vertices.push_back(bottomVertex);

  for (uint32_t i = 1; i <= sliceCount; ++i) {
    mesh->indices.push_back(0);
    mesh->indices.push_back(i + 1);
    mesh->indices.push_back(i);
  }

  uint32_t baseIndex = 1;
  uint32_t ringVertexCount = sliceCount + 1;
  for (uint32_t i = 0; i < stackCount - 2; ++i) {
    for (uint32_t j = 0; j < sliceCount; ++j) {
      mesh->indices.push_back(baseIndex + i * ringVertexCount + j);
      mesh->indices.push_back(baseIndex + i * ringVertexCount + j + 1);
      mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

      mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
      mesh->indices.push_back(baseIndex + i * ringVertexCount + j + 1);
      mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
    }
  }

  uint32_t southPoleIndex = static_cast<uint32_t>(mesh->vertices.size()) - 1;
  baseIndex = southPoleIndex - ringVertexCount;
  for (uint32_t i = 0; i < sliceCount; ++i) {
    mesh->indices.push_back(southPoleIndex);
    mesh->indices.push_back(baseIndex + i);
    mesh->indices.push_back(baseIndex + i + 1);
  }

  createBuffers(mesh);

  MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Created sphere with ID: ", id);

  return id;
}

MeshID MeshManager::createPlane(float width, float height) {
  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Creating plane (width: ", width,
             ", height: ", height, ")");

  Mesh* mesh = new Mesh();
  mesh->name = "Plane";
  mesh->type = MeshType::Plane;

  float halfW = width * 0.5f;
  float halfH = height * 0.5f;

  mesh->vertices = {
      {{-halfW, 0.0f, -halfH},
       {1.0f, 1.0f, 1.0f},
       {0.0f, 0.0f},
       {0.0f, 1.0f, 0.0f}},
      {{halfW, 0.0f, -halfH},
       {1.0f, 1.0f, 1.0f},
       {1.0f, 0.0f},
       {0.0f, 1.0f, 0.0f}},
      {{halfW, 0.0f, halfH},
       {1.0f, 1.0f, 1.0f},
       {1.0f, 1.0f},
       {0.0f, 1.0f, 0.0f}},
      {{-halfW, 0.0f, halfH},
       {1.0f, 1.0f, 1.0f},
       {0.0f, 1.0f},
       {0.0f, 1.0f, 0.0f}},
  };

  mesh->indices = {0, 2, 1, 2, 0, 3};

  createBuffers(mesh);

  MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Created plane with ID: ", id);

  return id;
}

MeshID MeshManager::createCylinder(float radius, float height,
                                   uint32_t segments) {
  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Creating cylinder (radius: ", radius,
             ", height: ", height, ", segments: ", segments, ")");
  Mesh* mesh = new Mesh();
  mesh->name = "Cylinder";
  mesh->type = MeshType::Cylinder;
  uint32_t sliceCount = segments;
  uint32_t stackCount = 1;
  float stackHeight = height / stackCount;
  uint32_t ringCount = stackCount + 1;
  for (uint32_t i = 0; i < ringCount; ++i) {
    float y = -0.5f * height + i * stackHeight;
    float dTheta = 2.0f * glm::pi<float>() / sliceCount;
    for (uint32_t j = 0; j <= sliceCount; ++j) {
      Vertex vertex;
      float c = cosf(j * dTheta);
      float s = sinf(j * dTheta);
      vertex.pos = glm::vec3(radius * c, y, radius * s);
      vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
      vertex.normal = glm::normalize(glm::vec3(c, 0.0f, s));
      vertex.texCoord = glm::vec2(static_cast<float>(j) / sliceCount,
                                  static_cast<float>(i) / stackCount);
      mesh->vertices.push_back(vertex);
    }
  }
  uint32_t ringVertexCount = sliceCount + 1;
  for (uint32_t i = 0; i < stackCount; ++i) {
    for (uint32_t j = 0; j < sliceCount; ++j) {
      mesh->indices.push_back(i * ringVertexCount + j);
      mesh->indices.push_back((i + 1) * ringVertexCount + j);
      mesh->indices.push_back((i + 1) * ringVertexCount + j + 1);
      mesh->indices.push_back(i * ringVertexCount + j);
      mesh->indices.push_back((i + 1) * ringVertexCount + j + 1);
      mesh->indices.push_back(i * ringVertexCount + j + 1);
    }
  }
  uint32_t baseIndex = static_cast<uint32_t>(mesh->vertices.size());
  float dTheta = 2.0f * glm::pi<float>() / sliceCount;
  for (uint32_t i = 0; i <= sliceCount; ++i) {
    float x = radius * cosf(i * dTheta);
    float z = radius * sinf(i * dTheta);
    float y = -0.5f * height;
    Vertex vertex;
    vertex.pos = glm::vec3(x, y, z);
    vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    vertex.texCoord =
        glm::vec2(x / radius * 0.5f + 0.5f, z / radius * 0.5f + 0.5f);
    mesh->vertices.push_back(vertex);
  }
  Vertex centerVertex;
  centerVertex.pos = glm::vec3(0.0f, -0.5f * height, 0.0f);
  centerVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  centerVertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
  centerVertex.texCoord = glm::vec2(0.5f, 0.5f);
  mesh->vertices.push_back(centerVertex);
  uint32_t centerIndex = static_cast<uint32_t>(mesh->vertices.size()) - 1;
  for (uint32_t i = 0; i < sliceCount; ++i) {
    mesh->indices.push_back(centerIndex);
    mesh->indices.push_back(baseIndex + i);
    mesh->indices.push_back(baseIndex + i + 1);
  }
  baseIndex = static_cast<uint32_t>(mesh->vertices.size());
  for (uint32_t i = 0; i <= sliceCount; ++i) {
    float x = radius * cosf(i * dTheta);
    float z = radius * sinf(i * dTheta);
    float y = 0.5f * height;
    Vertex vertex;
    vertex.pos = glm::vec3(x, y, z);
    vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    vertex.texCoord =
        glm::vec2(x / radius * 0.5f + 0.5f, z / radius * 0.5f + 0.5f);
    mesh->vertices.push_back(vertex);
  }
  centerVertex.pos = glm::vec3(0.0f, 0.5f * height, 0.0f);
  centerVertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
  mesh->vertices.push_back(centerVertex);
  centerIndex = static_cast<uint32_t>(mesh->vertices.size()) - 1;
  for (uint32_t i = 0; i < sliceCount; ++i) {
    mesh->indices.push_back(centerIndex);
    mesh->indices.push_back(baseIndex + i + 1);
    mesh->indices.push_back(baseIndex + i);
  }
  createBuffers(mesh);
  MeshID id = registerMesh(mesh);
  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Created cylinder with ID: ", id);
  return id;
}

MeshID MeshManager::createParticleQuad() {
  Debug::log(Debug::Category::RENDERING, "MeshManager: Creating particle quad");

  Mesh* mesh = new Mesh();
  mesh->name = "Particle Quad";
  mesh->type = MeshType::Plane;

  mesh->vertices = {
      {{-0.5f, -0.5f, 0.0f},
       {1.0f, 1.0f, 1.0f},
       {0.0f, 0.0f},
       {0.0f, 0.0f, 1.0f}},
      {{0.5f, -0.5f, 0.0f},
       {1.0f, 1.0f, 1.0f},
       {1.0f, 0.0f},
       {0.0f, 0.0f, 1.0f}},
      {{0.5f, 0.5f, 0.0f},
       {1.0f, 1.0f, 1.0f},
       {1.0f, 1.0f},
       {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f},
       {1.0f, 1.0f, 1.0f},
       {0.0f, 1.0f},
       {0.0f, 0.0f, 1.0f}},
  };

  mesh->indices = {0, 1, 2, 2, 3, 0};

  createBuffers(mesh);
  MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Created particle quad with ID: ", id);
  return id;
}

MeshID MeshManager::loadFromOBJ(const std::string& filepath) {
  auto it = filepathToID.find(filepath);
  if (it != filepathToID.end()) {
    Debug::log(Debug::Category::RENDERING,
               "MeshManager: OBJ already loaded: ", filepath,
               " (ID: ", it->second, ")");
    return it->second;
  }

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Loading OBJ: ", filepath);

  std::ifstream file(filepath);
  if (!file.is_open()) {
    Debug::log(Debug::Category::RENDERING,
               "MeshManager: Failed to open OBJ file: ", filepath,
               ", returning default cube");
    return defaultCubeID;
  }

  Mesh* mesh = new Mesh();
  mesh->name = filepath.substr(filepath.find_last_of("/\\") + 1);
  mesh->type = MeshType::Custom;

  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> texCoords;

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "v") {
      glm::vec3 pos;
      iss >> pos.x >> pos.y >> pos.z;
      positions.push_back(pos);
    } else if (prefix == "vn") {
      glm::vec3 normal;
      iss >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    } else if (prefix == "vt") {
      glm::vec2 texCoord;
      iss >> texCoord.x >> texCoord.y;
      texCoords.push_back(texCoord);
    } else if (prefix == "f") {
      std::string vertexData;
      std::vector<uint16_t> faceIndices;

      while (iss >> vertexData) {
        std::istringstream viss(vertexData);
        std::string indexStr;
        int posIdx = 0, texIdx = 0, normIdx = 0;

        if (std::getline(viss, indexStr, '/')) {
          posIdx = std::stoi(indexStr) - 1;
        }
        if (std::getline(viss, indexStr, '/')) {
          if (!indexStr.empty()) {
            texIdx = std::stoi(indexStr) - 1;
          }
        }
        if (std::getline(viss, indexStr, '/')) {
          if (!indexStr.empty()) {
            normIdx = std::stoi(indexStr) - 1;
          }
        }

        Vertex vertex;
        vertex.pos = positions[posIdx];
        vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
        vertex.texCoord =
            (texIdx >= 0 && texIdx < static_cast<int>(texCoords.size()))
                ? texCoords[texIdx]
                : glm::vec2(0.0f, 0.0f);
        vertex.normal =
            (normIdx >= 0 && normIdx < static_cast<int>(normals.size()))
                ? normals[normIdx]
                : glm::vec3(0.0f, 1.0f, 0.0f);

        mesh->vertices.push_back(vertex);
        faceIndices.push_back(static_cast<uint16_t>(mesh->vertices.size() - 1));
      }

      if (faceIndices.size() == 3) {
        mesh->indices.push_back(faceIndices[0]);
        mesh->indices.push_back(faceIndices[1]);
        mesh->indices.push_back(faceIndices[2]);
      } else if (faceIndices.size() == 4) {
        mesh->indices.push_back(faceIndices[0]);
        mesh->indices.push_back(faceIndices[1]);
        mesh->indices.push_back(faceIndices[2]);

        mesh->indices.push_back(faceIndices[0]);
        mesh->indices.push_back(faceIndices[2]);
        mesh->indices.push_back(faceIndices[3]);
      }
    }
  }

  file.close();

  if (!mesh->vertices.empty()) {
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (const auto& vertex : mesh->vertices) {
      minBounds.x = std::min(minBounds.x, vertex.pos.x);
      minBounds.y = std::min(minBounds.y, vertex.pos.y);
      minBounds.z = std::min(minBounds.z, vertex.pos.z);

      maxBounds.x = std::max(maxBounds.x, vertex.pos.x);
      maxBounds.y = std::max(maxBounds.y, vertex.pos.y);
      maxBounds.z = std::max(maxBounds.z, vertex.pos.z);
    }

    glm::vec3 center = (minBounds + maxBounds) * 0.5f;

    Debug::log(Debug::Category::RENDERING,
               "MeshManager: Centering mesh - offset: (", center.x, ", ",
               center.y, ", ", center.z, ")");

    for (auto& vertex : mesh->vertices) {
      vertex.pos -= center;
    }
  }

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: OBJ loaded successfully - vertices: ",
             mesh->vertices.size(), ", indices: ", mesh->indices.size());

  createBuffers(mesh);
  MeshID id = registerMesh(mesh);
  filepathToID[filepath] = id;

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Registered OBJ with ID: ", id);

  return id;
}

Mesh* MeshManager::getMesh(MeshID id) {
  if (id >= meshes.size() || meshes[id] == nullptr) {
    Debug::log(Debug::Category::RENDERING, "MeshManager: Invalid mesh ID: ", id,
               ", returning default cube");
    return meshes[defaultCubeID].get();
  }
  return meshes[id].get();
}

const Mesh* MeshManager::getMesh(MeshID id) const {
  if (id >= meshes.size() || meshes[id] == nullptr) {
    Debug::log(Debug::Category::RENDERING,
               "MeshManager: Invalid mesh ID (const): ", id,
               ", returning default cube");
    return meshes[defaultCubeID].get();
  }
  return meshes[id].get();
}

void MeshManager::cleanup() {
  Debug::log(Debug::Category::RENDERING, "MeshManager: Cleaning up ",
             meshes.size(), " meshes");

  VkDevice device = renderDevice->getDevice();

  for (auto& mesh : meshes) {
    if (mesh) {
      if (mesh->vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mesh->vertexBuffer, nullptr);
      }
      if (mesh->vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, mesh->vertexBufferMemory, nullptr);
      }
      if (mesh->indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mesh->indexBuffer, nullptr);
      }
      if (mesh->indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, mesh->indexBufferMemory, nullptr);
      }
    }
  }

  meshes.clear();

  Debug::log(Debug::Category::RENDERING, "MeshManager: Cleanup complete");
}

MeshID MeshManager::registerMesh(Mesh* mesh) {
  if (!mesh) {
    Debug::log(Debug::Category::RENDERING,
               "MeshManager: Attempted to register null mesh!");
    return defaultCubeID;
  }

  MeshID id = static_cast<MeshID>(meshes.size());
  meshes.push_back(std::unique_ptr<Mesh>(mesh));

  Debug::log(Debug::Category::RENDERING, "MeshManager: Registered mesh '",
             mesh->name, "' (ID: ", id, ", vertices: ", mesh->vertices.size(),
             ", indices: ", mesh->indices.size(), ")");

  return id;
}

MeshID MeshManager::createProceduralTerrain(float radius, uint32_t segments,
                                            float heightScale, float noiseScale,
                                            int octaves, float persistence,
                                            unsigned int seed) {
  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Creating circular procedural terrain (radius: ",
             radius, ", segments: ", segments, ")");

  Mesh* mesh = new Mesh();
  mesh->name = "Procedural Terrain";
  mesh->type = MeshType::Plane;

  PerlinNoise perlin(seed);

  uint32_t radialSegments = segments * 4;

  Vertex centerVertex;
  centerVertex.pos = glm::vec3(
      0.0f, perlin.octaveNoise(0.0f, 0.0f, octaves, persistence) * heightScale,
      0.0f);
  centerVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  centerVertex.texCoord = glm::vec2(0.5f, 0.5f);
  centerVertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
  mesh->vertices.push_back(centerVertex);

  for (uint32_t ring = 1; ring <= segments; ring++) {
    float ringRadius = (ring / (float)segments) * radius;

    for (uint32_t point = 0; point < radialSegments; point++) {
      float angle = (point / (float)radialSegments) * 2.0f * glm::pi<float>();

      float xPos = ringRadius * cos(angle);
      float zPos = ringRadius * sin(angle);

      float noiseX = (xPos / radius) * noiseScale;
      float noiseZ = (zPos / radius) * noiseScale;

      float height = perlin.octaveNoise(noiseX, noiseZ, octaves, persistence) *
                     heightScale;

      Vertex vertex;
      vertex.pos = glm::vec3(xPos, height, zPos);
      vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
      vertex.texCoord = glm::vec2((xPos / radius + 1.0f) * 0.5f,
                                  (zPos / radius + 1.0f) * 0.5f);
      vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);

      mesh->vertices.push_back(vertex);
    }
  }

  for (uint32_t point = 0; point < radialSegments; point++) {
    uint32_t nextPoint = (point + 1) % radialSegments;

    mesh->indices.push_back(0);
    mesh->indices.push_back(1 + nextPoint);
    mesh->indices.push_back(1 + point);
  }

  for (uint32_t ring = 1; ring < segments; ring++) {
    uint32_t currentRingStart = 1 + (ring - 1) * radialSegments;
    uint32_t nextRingStart = 1 + ring * radialSegments;

    for (uint32_t point = 0; point < radialSegments; point++) {
      uint32_t nextPoint = (point + 1) % radialSegments;

      uint32_t current = currentRingStart + point;
      uint32_t currentNext = currentRingStart + nextPoint;
      uint32_t next = nextRingStart + point;
      uint32_t nextNext = nextRingStart + nextPoint;

      mesh->indices.push_back(current);
      mesh->indices.push_back(nextNext);
      mesh->indices.push_back(next);

      mesh->indices.push_back(current);
      mesh->indices.push_back(currentNext);
      mesh->indices.push_back(nextNext);
    }
  }

  for (size_t i = 0; i < mesh->indices.size(); i += 3) {
    uint32_t idx0 = mesh->indices[i];
    uint32_t idx1 = mesh->indices[i + 1];
    uint32_t idx2 = mesh->indices[i + 2];

    glm::vec3 v0 = mesh->vertices[idx0].pos;
    glm::vec3 v1 = mesh->vertices[idx1].pos;
    glm::vec3 v2 = mesh->vertices[idx2].pos;

    glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

    mesh->vertices[idx0].normal += normal;
    mesh->vertices[idx1].normal += normal;
    mesh->vertices[idx2].normal += normal;
  }

  for (auto& vertex : mesh->vertices) {
    vertex.normal = glm::normalize(vertex.normal);
  }

  createBuffers(mesh);
  MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Created circular procedural terrain with ID: ", id,
             " (vertices: ", mesh->vertices.size(),
             ", indices: ", mesh->indices.size(), ")");

  return id;
}

void MeshManager::createBuffers(Mesh* mesh) {
  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Creating buffers for mesh '", mesh->name, "'");

  VkDevice device = renderDevice->getDevice();

  VkDeviceSize vertexBufferSize = sizeof(Vertex) * mesh->vertices.size();
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  renderDevice->createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
  memcpy(data, mesh->vertices.data(), static_cast<size_t>(vertexBufferSize));
  vkUnmapMemory(device, stagingBufferMemory);

  renderDevice->createBuffer(
      vertexBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh->vertexBuffer,
      mesh->vertexBufferMemory);

  renderDevice->copyBuffer(stagingBuffer, mesh->vertexBuffer, vertexBufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  VkDeviceSize indexBufferSize = sizeof(uint16_t) * mesh->indices.size();

  renderDevice->createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, stagingBufferMemory);

  vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
  memcpy(data, mesh->indices.data(), static_cast<size_t>(indexBufferSize));
  vkUnmapMemory(device, stagingBufferMemory);

  renderDevice->createBuffer(
      indexBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh->indexBuffer,
      mesh->indexBufferMemory);

  renderDevice->copyBuffer(stagingBuffer, mesh->indexBuffer, indexBufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  Debug::log(Debug::Category::RENDERING,
             "  - Created vertex and index buffers");
}

void MeshManager::createDefaultMeshes() {
  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Creating default meshes");

  defaultCubeID = createCube(1.0f);

  Debug::log(Debug::Category::RENDERING,
             "MeshManager: Default cube created with ID: ", defaultCubeID);
}