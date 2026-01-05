#include "MeshManager.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include "Util/Debug.h"
#include "Util/PerlinNoise.h"

const VkVertexInputBindingDescription& Vertex::getBindingDescription() {
  static VkVertexInputBindingDescription const bindingDescription = {
      0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
  return bindingDescription;
}

const std::array<VkVertexInputAttributeDescription, 4>&
Vertex::getAttributeDescriptions() {
  static std::array<VkVertexInputAttributeDescription, 4> const
      attributeDescriptions = {
          {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
           {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
           {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)},
           {0, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)}}};
  return attributeDescriptions;
}

MeshManager::MeshManager(RenderDevice* renderDev)
    : renderDevice(renderDev), defaultCubeID(0) {
  Debug::log(Debug::Category::MESH, "MeshManager: Constructor called");

  meshes.push_back(std::unique_ptr<Mesh>(nullptr));

  createDefaultMeshes();

  Debug::log(Debug::Category::MESH, "MeshManager: Initialization complete");
}

MeshManager::~MeshManager() noexcept {
  try {
    Debug::log(Debug::Category::MESH, "MeshManager: Destructor called");
    cleanup();
  } catch (...) {
  }
}

MeshID MeshManager::createCube(float size) {
  Debug::log(Debug::Category::MESH, "MeshManager: Creating cube (size: ", size,
             ")");

  Mesh* const mesh = new Mesh();
  mesh->setName("Cube");
  mesh->setType(MeshType::Cube);

  const float h = size * 0.5f;

  std::vector<Vertex> vertices = {
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

  mesh->setVertices(vertices);

  std::vector<uint16_t> indices = {
      0,  2,  1,  0,  3,  2,  4,  6,  5,  4,  7,  6,  8,  10, 9,  8,  11, 10,
      12, 14, 13, 12, 15, 14, 16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22};
  mesh->setIndices(indices);

  createBuffers(mesh);

  const MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::MESH, "MeshManager: Created cube with ID: ", id);

  return id;
}

MeshID MeshManager::createSphere(float radius, uint32_t segments) {
  Debug::log(Debug::Category::MESH,
             "MeshManager: Creating sphere (radius: ", radius,
             ", segments: ", segments, ")");

  Mesh* const mesh = new Mesh();
  mesh->setName("Sphere");
  mesh->setType(MeshType::Sphere);

  const uint32_t sliceCount = segments;
  const uint32_t stackCount = segments;

  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  Vertex topVertex{};
  topVertex.pos = glm::vec3(0.0f, radius, 0.0f);
  topVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  topVertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
  topVertex.texCoord = glm::vec2(0.0f, 0.0f);
  vertices.push_back(topVertex);

  const float phiStep = glm::pi<float>() / stackCount;
  const float thetaStep = 2.0f * glm::pi<float>() / sliceCount;

  for (uint32_t i = 1; i <= stackCount - 1; ++i) {
    const float phi = i * phiStep;
    for (uint32_t j = 0; j <= sliceCount; ++j) {
      const float theta = j * thetaStep;

      Vertex v{};
      v.pos.x = radius * sinf(phi) * cosf(theta);
      v.pos.y = radius * cosf(phi);
      v.pos.z = radius * sinf(phi) * sinf(theta);
      v.color = glm::vec3(1.0f, 1.0f, 1.0f);
      v.normal = glm::normalize(v.pos);
      v.texCoord = glm::vec2(static_cast<float>(j) / sliceCount,
                             static_cast<float>(i) / stackCount);

      vertices.push_back(v);
    }
  }

  Vertex bottomVertex{};
  bottomVertex.pos = glm::vec3(0.0f, -radius, 0.0f);
  bottomVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  bottomVertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
  bottomVertex.texCoord = glm::vec2(0.0f, 1.0f);
  vertices.push_back(bottomVertex);

  for (uint32_t i = 1; i <= sliceCount; ++i) {
    indices.push_back(0);
    indices.push_back(i + 1);
    indices.push_back(i);
  }

  const uint32_t baseIndex = 1;
  const uint32_t ringVertexCount = sliceCount + 1;
  for (uint32_t i = 0; i < stackCount - 2; ++i) {
    for (uint32_t j = 0; j < sliceCount; ++j) {
      indices.push_back(baseIndex + i * ringVertexCount + j);
      indices.push_back(baseIndex + i * ringVertexCount + j + 1);
      indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

      indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
      indices.push_back(baseIndex + i * ringVertexCount + j + 1);
      indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
    }
  }

  const uint32_t southPoleIndex = static_cast<uint32_t>(vertices.size()) - 1;
  const uint32_t southBaseIndex = southPoleIndex - ringVertexCount;
  for (uint32_t i = 0; i < sliceCount; ++i) {
    indices.push_back(southPoleIndex);
    indices.push_back(southBaseIndex + i);
    indices.push_back(southBaseIndex + i + 1);
  }

  mesh->setVertices(vertices);
  mesh->setIndices(indices);

  createBuffers(mesh);

  const MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::MESH,
             "MeshManager: Created sphere with ID: ", id);

  return id;
}

MeshID MeshManager::createPlane(float width, float height) {
  Debug::log(Debug::Category::MESH,
             "MeshManager: Creating plane (width: ", width,
             ", height: ", height, ")");

  Mesh* const mesh = new Mesh();
  mesh->setName("Plane");
  mesh->setType(MeshType::Plane);

  const float halfW = width * 0.5f;
  const float halfH = height * 0.5f;

  std::vector<Vertex> vertices = {
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

  mesh->setVertices(vertices);

  std::vector<uint16_t> indices = {0, 2, 1, 2, 0, 3};
  mesh->setIndices(indices);

  createBuffers(mesh);

  const MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::MESH, "MeshManager: Created plane with ID: ", id);

  return id;
}

MeshID MeshManager::createCylinder(float radius, float height,
                                   uint32_t segments) {
  Debug::log(Debug::Category::MESH,
             "MeshManager: Creating cylinder (radius: ", radius,
             ", height: ", height, ", segments: ", segments, ")");
  Mesh* const mesh = new Mesh();
  mesh->setName("Cylinder");
  mesh->setType(MeshType::Cylinder);
  const uint32_t sliceCount = segments;
  const uint32_t stackCount = 1;
  const float stackHeight = height / stackCount;
  const uint32_t ringCount = stackCount + 1;

  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  for (uint32_t i = 0; i < ringCount; ++i) {
    const float y = -0.5f * height + i * stackHeight;
    const float dTheta = 2.0f * glm::pi<float>() / sliceCount;
    for (uint32_t j = 0; j <= sliceCount; ++j) {
      Vertex vertex{};
      const float c = cosf(j * dTheta);
      const float s = sinf(j * dTheta);
      vertex.pos = glm::vec3(radius * c, y, radius * s);
      vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
      vertex.normal = glm::normalize(glm::vec3(c, 0.0f, s));
      vertex.texCoord = glm::vec2(static_cast<float>(j) / sliceCount,
                                  static_cast<float>(i) / stackCount);
      vertices.push_back(vertex);
    }
  }
  const uint32_t ringVertexCount = sliceCount + 1;
  for (uint32_t i = 0; i < stackCount; ++i) {
    for (uint32_t j = 0; j < sliceCount; ++j) {
      indices.push_back(i * ringVertexCount + j);
      indices.push_back((i + 1) * ringVertexCount + j);
      indices.push_back((i + 1) * ringVertexCount + j + 1);
      indices.push_back(i * ringVertexCount + j);
      indices.push_back((i + 1) * ringVertexCount + j + 1);
      indices.push_back(i * ringVertexCount + j + 1);
    }
  }
  uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
  const float dTheta = 2.0f * glm::pi<float>() / sliceCount;
  for (uint32_t i = 0; i <= sliceCount; ++i) {
    const float x = radius * cosf(i * dTheta);
    const float z = radius * sinf(i * dTheta);
    const float y = -0.5f * height;
    Vertex vertex{};
    vertex.pos = glm::vec3(x, y, z);
    vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    vertex.texCoord =
        glm::vec2(x / radius * 0.5f + 0.5f, z / radius * 0.5f + 0.5f);
    vertices.push_back(vertex);
  }
  Vertex centerVertex{};
  centerVertex.pos = glm::vec3(0.0f, -0.5f * height, 0.0f);
  centerVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  centerVertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
  centerVertex.texCoord = glm::vec2(0.5f, 0.5f);
  vertices.push_back(centerVertex);
  uint32_t centerIndex = static_cast<uint32_t>(vertices.size()) - 1;
  for (uint32_t i = 0; i < sliceCount; ++i) {
    indices.push_back(centerIndex);
    indices.push_back(baseIndex + i);
    indices.push_back(baseIndex + i + 1);
  }
  baseIndex = static_cast<uint32_t>(vertices.size());
  for (uint32_t i = 0; i <= sliceCount; ++i) {
    const float x = radius * cosf(i * dTheta);
    const float z = radius * sinf(i * dTheta);
    const float y = 0.5f * height;
    Vertex vertex{};
    vertex.pos = glm::vec3(x, y, z);
    vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    vertex.texCoord =
        glm::vec2(x / radius * 0.5f + 0.5f, z / radius * 0.5f + 0.5f);
    vertices.push_back(vertex);
  }
  centerVertex = Vertex{};
  centerVertex.pos = glm::vec3(0.0f, 0.5f * height, 0.0f);
  centerVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  centerVertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
  centerVertex.texCoord = glm::vec2(0.5f, 0.5f);
  vertices.push_back(centerVertex);
  centerIndex = static_cast<uint32_t>(vertices.size()) - 1;
  for (uint32_t i = 0; i < sliceCount; ++i) {
    indices.push_back(centerIndex);
    indices.push_back(baseIndex + i + 1);
    indices.push_back(baseIndex + i);
  }

  mesh->setVertices(vertices);
  mesh->setIndices(indices);

  createBuffers(mesh);
  const MeshID id = registerMesh(mesh);
  Debug::log(Debug::Category::MESH,
             "MeshManager: Created cylinder with ID: ", id);
  return id;
}

MeshID MeshManager::createParticleQuad() {
  Debug::log(Debug::Category::MESH, "MeshManager: Creating particle quad");

  Mesh* const mesh = new Mesh();
  mesh->setName("Particle Quad");
  mesh->setType(MeshType::Plane);

  std::vector<Vertex> vertices = {
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
  mesh->setVertices(vertices);

  std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
  mesh->setIndices(indices);

  createBuffers(mesh);
  const MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::MESH,
             "MeshManager: Created particle quad with ID: ", id);
  return id;
}

MeshID MeshManager::loadFromOBJ(const std::string& filepath) {
  const auto it = filepathToID.find(filepath);
  if (it != filepathToID.end()) {
    Debug::log(Debug::Category::MESH,
               "MeshManager: OBJ already loaded: ", filepath,
               " (ID: ", it->second, ")");
    return it->second;
  }

  Debug::log(Debug::Category::MESH, "MeshManager: Loading OBJ: ", filepath);

  std::ifstream file(filepath);
  if (!file.is_open()) {
    Debug::log(Debug::Category::MESH,
               "MeshManager: Failed to open OBJ file: ", filepath,
               ", returning default cube");
    return defaultCubeID;
  }

  Mesh* const mesh = new Mesh();
  mesh->setName(filepath.substr(filepath.find_last_of("/\\") + 1));
  mesh->setType(MeshType::Custom);

  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> texCoords;

  std::vector<Vertex> meshVertices;
  std::vector<uint16_t> meshIndices;

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "v") {
      glm::vec3 pos{};
      iss >> pos.x >> pos.y >> pos.z;
      positions.push_back(pos);
    } else if (prefix == "vn") {
      glm::vec3 normal{};
      iss >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    } else if (prefix == "vt") {
      glm::vec2 texCoord{};
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

        Vertex vertex{};
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

        meshVertices.push_back(vertex);
        faceIndices.push_back(static_cast<uint16_t>(meshVertices.size() - 1));
      }

      if (faceIndices.size() == 3) {
        meshIndices.push_back(faceIndices[0]);
        meshIndices.push_back(faceIndices[1]);
        meshIndices.push_back(faceIndices[2]);
      } else if (faceIndices.size() == 4) {
        meshIndices.push_back(faceIndices[0]);
        meshIndices.push_back(faceIndices[1]);
        meshIndices.push_back(faceIndices[2]);

        meshIndices.push_back(faceIndices[0]);
        meshIndices.push_back(faceIndices[2]);
        meshIndices.push_back(faceIndices[3]);
      }
    }
  }

  file.close();

  if (!meshVertices.empty()) {
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (const auto& vertex : meshVertices) {
      minBounds.x = std::min(minBounds.x, vertex.pos.x);
      minBounds.y = std::min(minBounds.y, vertex.pos.y);
      minBounds.z = std::min(minBounds.z, vertex.pos.z);

      maxBounds.x = std::max(maxBounds.x, vertex.pos.x);
      maxBounds.y = std::max(maxBounds.y, vertex.pos.y);
      maxBounds.z = std::max(maxBounds.z, vertex.pos.z);
    }

    const glm::vec3 center = (minBounds + maxBounds) * 0.5f;

    Debug::log(Debug::Category::MESH, "MeshManager: Centering mesh - offset: (",
               center.x, ", ", center.y, ", ", center.z, ")");

    for (auto& vertex : meshVertices) {
      vertex.pos -= center;
    }
  }

  mesh->setVertices(meshVertices);
  mesh->setIndices(meshIndices);

  Debug::log(Debug::Category::MESH,
             "MeshManager: OBJ loaded successfully - vertices: ",
             meshVertices.size(), ", indices: ", meshIndices.size());

  createBuffers(mesh);
  const MeshID id = registerMesh(mesh);
  filepathToID[filepath] = id;

  Debug::log(Debug::Category::MESH,
             "MeshManager: Registered OBJ with ID: ", id);

  return id;
}

Mesh* MeshManager::getMesh(MeshID id) {
  if (id >= meshes.size() || meshes[id] == nullptr) {
    Debug::log(Debug::Category::MESH, "MeshManager: Invalid mesh ID: ", id,
               ", returning default cube");
    return meshes[defaultCubeID].get();
  }
  return meshes[id].get();
}

const Mesh* MeshManager::getMesh(MeshID id) const {
  if (id >= meshes.size() || meshes[id] == nullptr) {
    Debug::log(Debug::Category::MESH,
               "MeshManager: Invalid mesh ID (const): ", id,
               ", returning default cube");
    return meshes[defaultCubeID].get();
  }
  return meshes[id].get();
}

void MeshManager::cleanup() {
  Debug::log(Debug::Category::MESH, "MeshManager: Cleaning up ", meshes.size(),
             " meshes");

  const VkDevice device = renderDevice->getDevice();

  for (auto& mesh : meshes) {
    if (mesh) {
      if (mesh->getVertexBuffer() != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mesh->getVertexBuffer(), nullptr);
      }
      if (mesh->getVertexBufferMemory() != VK_NULL_HANDLE) {
        vkFreeMemory(device, mesh->getVertexBufferMemory(), nullptr);
      }
      if (mesh->getIndexBuffer() != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mesh->getIndexBuffer(), nullptr);
      }
      if (mesh->getIndexBufferMemory() != VK_NULL_HANDLE) {
        vkFreeMemory(device, mesh->getIndexBufferMemory(), nullptr);
      }
    }
  }

  meshes.clear();

  Debug::log(Debug::Category::MESH, "MeshManager: Cleanup complete");
}

MeshID MeshManager::registerMesh(Mesh* mesh) {
  if (!mesh) {
    Debug::log(Debug::Category::MESH,
               "MeshManager: Attempted to register null mesh!");
    return defaultCubeID;
  }

  const MeshID id = static_cast<MeshID>(meshes.size());
  std::vector<Vertex> vertices;
  mesh->getVertices(vertices);
  std::vector<uint16_t> indices;
  mesh->getIndices(indices);
  std::string name;
  mesh->getName(name);

  meshes.push_back(std::unique_ptr<Mesh>(mesh));

  Debug::log(Debug::Category::MESH, "MeshManager: Registered mesh '", name,
             "' (ID: ", id, ", vertices: ", vertices.size(),
             ", indices: ", indices.size(), ")");

  return id;
}

MeshID MeshManager::createProceduralTerrain(float radius, uint32_t segments,
                                            float heightScale, float noiseScale,
                                            int octaves, float persistence,
                                            unsigned int seed) {
  Debug::log(Debug::Category::MESH,
             "MeshManager: Creating circular procedural terrain (radius: ",
             radius, ", segments: ", segments, ")");

  Mesh* const mesh = new Mesh();
  mesh->setName("Procedural Terrain");
  mesh->setType(MeshType::Plane);

  const PerlinNoise perlin(seed);

  const uint32_t radialSegments = segments * 4;

  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  Vertex centerVertex{};
  centerVertex.pos = glm::vec3(
      0.0f, perlin.octaveNoise(0.0f, 0.0f, octaves, persistence) * heightScale,
      0.0f);
  centerVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
  centerVertex.texCoord = glm::vec2(0.5f, 0.5f);
  centerVertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
  vertices.push_back(centerVertex);

  for (uint32_t ring = 1; ring <= segments; ring++) {
    const float ringRadius =
        static_cast<float>(ring) / static_cast<float>(segments) * radius;

    for (uint32_t point = 0; point < radialSegments; point++) {
      const float angle = static_cast<float>(point) /
                          static_cast<float>(radialSegments) * 2.0f *
                          glm::pi<float>();

      const float xPos = ringRadius * cos(angle);
      const float zPos = ringRadius * sin(angle);

      const float noiseX = (xPos / radius) * noiseScale;
      const float noiseZ = (zPos / radius) * noiseScale;

      const float height =
          perlin.octaveNoise(noiseX, noiseZ, octaves, persistence) *
          heightScale;

      Vertex vertex{};
      vertex.pos = glm::vec3(xPos, height, zPos);
      vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
      vertex.texCoord = glm::vec2((xPos / radius + 1.0f) * 0.5f,
                                  (zPos / radius + 1.0f) * 0.5f);
      vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);

      vertices.push_back(vertex);
    }
  }

  for (uint32_t point = 0; point < radialSegments; point++) {
    const uint32_t nextPoint = (point + 1) % radialSegments;

    indices.push_back(0);
    indices.push_back(1 + nextPoint);
    indices.push_back(1 + point);
  }

  for (uint32_t ring = 1; ring < segments; ring++) {
    const uint32_t currentRingStart = 1 + (ring - 1) * radialSegments;
    const uint32_t nextRingStart = 1 + ring * radialSegments;

    for (uint32_t point = 0; point < radialSegments; point++) {
      const uint32_t nextPoint = (point + 1) % radialSegments;

      const uint32_t current = currentRingStart + point;
      const uint32_t currentNext = currentRingStart + nextPoint;
      const uint32_t next = nextRingStart + point;
      const uint32_t nextNext = nextRingStart + nextPoint;

      indices.push_back(current);
      indices.push_back(nextNext);
      indices.push_back(next);

      indices.push_back(current);
      indices.push_back(currentNext);
      indices.push_back(nextNext);
    }
  }

  for (size_t i = 0; i < indices.size(); i += 3) {
    const uint32_t idx0 = indices[i];
    const uint32_t idx1 = indices[i + 1];
    const uint32_t idx2 = indices[i + 2];

    const glm::vec3 v0 = vertices[idx0].pos;
    const glm::vec3 v1 = vertices[idx1].pos;
    const glm::vec3 v2 = vertices[idx2].pos;

    const glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

    vertices[idx0].normal += normal;
    vertices[idx1].normal += normal;
    vertices[idx2].normal += normal;
  }

  for (auto& vertex : vertices) {
    vertex.normal = glm::normalize(vertex.normal);
  }

  mesh->setVertices(vertices);
  mesh->setIndices(indices);

  createBuffers(mesh);
  const MeshID id = registerMesh(mesh);

  Debug::log(Debug::Category::MESH,
             "MeshManager: Created circular procedural terrain with ID: ", id,
             " (vertices: ", vertices.size(), ", indices: ", indices.size(),
             ")");

  return id;
}

void MeshManager::createBuffers(Mesh* mesh) const {
  std::string name;
  mesh->getName(name);
  Debug::log(Debug::Category::MESH, "MeshManager: Creating buffers for mesh '",
             name, "'");

  const VkDevice device = renderDevice->getDevice();

  std::vector<Vertex> vertices;
  mesh->getVertices(vertices);
  std::vector<uint16_t> indices;
  mesh->getIndices(indices);

  const VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  renderDevice->createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
  memcpy(data, vertices.data(), static_cast<size_t>(vertexBufferSize));
  vkUnmapMemory(device, stagingBufferMemory);

  VkBuffer vBuf;
  VkDeviceMemory vMem;

  renderDevice->createBuffer(
      vertexBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vBuf, vMem);

  renderDevice->copyBuffer(stagingBuffer, vBuf, vertexBufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  mesh->setVertexBuffer(vBuf);
  mesh->setVertexBufferMemory(vMem);

  const VkDeviceSize indexBufferSize = sizeof(uint16_t) * indices.size();

  renderDevice->createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, stagingBufferMemory);

  vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
  memcpy(data, indices.data(), static_cast<size_t>(indexBufferSize));
  vkUnmapMemory(device, stagingBufferMemory);

  VkBuffer iBuf;
  VkDeviceMemory iMem;

  renderDevice->createBuffer(
      indexBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, iBuf, iMem);

  renderDevice->copyBuffer(stagingBuffer, iBuf, indexBufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  mesh->setIndexBuffer(iBuf);
  mesh->setIndexBufferMemory(iMem);

  Debug::log(Debug::Category::MESH, "  - Created vertex and index buffers");
}

void MeshManager::createDefaultMeshes() {
  Debug::log(Debug::Category::MESH, "MeshManager: Creating default meshes");

  defaultCubeID = createCube(1.0f);

  Debug::log(Debug::Category::MESH,
             "MeshManager: Default cube created with ID: ", defaultCubeID);
}