#include "PlantManager.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Util/Debug.h"

PlantManager::PlantManager(MeshManager* meshMgr, MaterialManager* materialMgr)
    : meshManager(meshMgr),
      materialManager(materialMgr),
      cactusMeshes(3),
      cactusMaterials(3) {
  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Constructor called");
}

PlantManager::~PlantManager() noexcept {
  try {
    Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Destructor called");
  } catch (...) {
  }
}

void PlantManager::init() {
  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Initializing");

  loadCactiModels();
  loadTreeModels();

  Debug::log(Debug::Category::PLANTMANAGER,
             "PlantManager: Initialization complete");
}

void PlantManager::loadCactiModels() {
  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Loading cactus models");

  struct CactusModelPair {
    std::string objPath;
    std::string mtlPath;
  };

  const std::vector<CactusModelPair> stage0Models = {
      {"./Models/Cacti/cacti2.obj", "./Models/Cacti/cacti2.mtl"},
      {"./Models/Cacti/cacti2v2.obj", "./Models/Cacti/cacti2v2.mtl"}};

  const std::vector<CactusModelPair> stage1Models = {
      {"./Models/Cacti/cacti3v1.obj", "./Models/Cacti/cacti3v1.mtl"},
      {"./Models/Cacti/cacti3v2.obj", "./Models/Cacti/cacti3v2.mtl"},
      {"./Models/Cacti/cacti3v3.obj", "./Models/Cacti/cacti3v3.mtl"}};

  for (const auto& model : stage0Models) {
    const MeshID meshID = meshManager->loadFromOBJ(model.objPath);
    const MaterialID matID = materialManager->loadFromMTL(model.mtlPath);
    cactusMeshes[0].push_back(meshID);
    cactusMaterials[0].push_back(matID);
  }

  for (const auto& model : stage1Models) {
    const MeshID meshID = meshManager->loadFromOBJ(model.objPath);
    const MaterialID matID = materialManager->loadFromMTL(model.mtlPath);
    cactusMeshes[1].push_back(meshID);
    cactusMaterials[1].push_back(matID);
  }

  cactusMeshes[2] = cactusMeshes[1];
  cactusMaterials[2] = cactusMaterials[1];

  Debug::log(
      Debug::Category::PLANTMANAGER,
      "PlantManager: Loaded cactus models - Stage 0: ", cactusMeshes[0].size(),
      " variants, Stage 1: ", cactusMeshes[1].size(),
      " variants, Stage 2: ", cactusMeshes[2].size(), " variants");
}

void PlantManager::loadTreeModels() {
  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Loading tree models");

  for (int i = 1; i <= 8; i++) {
    const std::string objPath =
        "./Models/Trees/jt" + std::to_string(i) + ".obj";
    const std::string mtlPath =
        "./Models/Trees/jt" + std::to_string(i) + ".mtl";

    const MeshID meshID = meshManager->loadFromOBJ(objPath);
    const MaterialID matID = materialManager->loadFromMTL(mtlPath);

    treeMeshes.push_back(meshID);
    treeMaterials.push_back(matID);
  }

  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Loaded ",
             treeMeshes.size(), " tree growth stages");
}

float PlantManager::getTerrainHeightAt(const Mesh* terrainMesh, float x,
                                       float z) const {
  float closestDistSq = std::numeric_limits<float>::max();
  float height = 0.0f;

  for (const auto& vertex : terrainMesh->vertices) {
    const float dx = vertex.pos.x - x;
    const float dz = vertex.pos.z - z;
    const float distSq = dx * dx + dz * dz;

    if (distSq < closestDistSq) {
      closestDistSq = distSq;
      height = vertex.pos.y;
    }
  }

  return height;
}

glm::vec3 PlantManager::getTerrainNormalAt(const Mesh* terrainMesh, float x,
                                           float z) const {
  float closestDistSq = std::numeric_limits<float>::max();
  glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);

  for (const auto& vertex : terrainMesh->vertices) {
    const float dx = vertex.pos.x - x;
    const float dz = vertex.pos.z - z;
    const float distSq = dx * dx + dz * dz;

    if (distSq < closestDistSq) {
      closestDistSq = distSq;
      normal = vertex.normal;
    }
  }

  return glm::normalize(normal);
}

void PlantManager::spawnPlantsOnTerrain(std::vector<Object>& sceneObjects,
                                        const Mesh* terrainMesh,
                                        const PlantSpawnConfig& config) {
  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Spawning ",
             config.numCacti, " cacti and ", config.numTrees,
             " trees with seed ", config.seed);

  rng.seed(config.seed);

  std::uniform_real_distribution<float> radiusDist(config.minRadius,
                                                   config.maxRadius);
  std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
  std::uniform_real_distribution<float> rotationDist(0.0f, 360.0f);
  std::uniform_real_distribution<float> varianceDist(-1.0f, 1.0f);
  std::uniform_real_distribution<float> sinkDist(0.2f, 0.5f);

  for (int i = 0; i < config.numCacti; i++) {
    const float radius = radiusDist(rng);
    const float angle = angleDist(rng);
    const float x = radius * std::cos(angle);
    const float z = radius * std::sin(angle);

    const float y = getTerrainHeightAt(terrainMesh, x, z);
    const glm::vec3 terrainNormal = getTerrainNormalAt(terrainMesh, x, z);

    int stage = 0;
    if (config.randomGrowthStages) {
      std::uniform_int_distribution<int> stageDist(0, 2);
      stage = stageDist(rng);
    }

    std::uniform_int_distribution<int> variantDist(
        0, static_cast<int>(cactusMeshes[stage].size()) - 1);
    const int variant = variantDist(rng);

    const float baseYaw = rotationDist(rng);
    const float rotationVariance =
        varianceDist(rng) * config.rotationVariance * 15.0f;
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 rotationAxis =
        glm::normalize(glm::cross(up, terrainNormal));
    const float tiltAngle =
        std::acos(glm::clamp(glm::dot(up, terrainNormal), -1.0f, 1.0f));
    const float pitch =
        tiltAngle * config.rotationVariance * (180.0f / glm::pi<float>());
    const float roll = 0.0f;
    const float yaw = baseYaw + rotationVariance;

    const float baseScale = 1.0f;
    const float scaleVarianceX = varianceDist(rng) * config.scaleVariance;
    const float scaleVarianceY = varianceDist(rng) * config.scaleVariance;
    const float scaleVarianceZ = varianceDist(rng) * config.scaleVariance;
    const float scaleX = baseScale + scaleVarianceX;
    const float scaleY = baseScale + scaleVarianceY;
    const float scaleZ = baseScale + scaleVarianceZ;

    const Mesh* mesh = meshManager->getMesh(cactusMeshes[stage][variant]);
    const float yOffset = calculateMeshBottomOffset(mesh);
    const float scaledYOffset = yOffset * scaleY;
    const float sinkAmount = sinkDist(rng);

    const Object cactusObj = ObjectBuilder()
                                 .name("Cactus_" + std::to_string(i))
                                 .position(x, y - scaledYOffset - sinkAmount, z)
                                 .rotationEuler(pitch, yaw, roll)
                                 .scale(scaleX, scaleY, scaleZ)
                                 .mesh(cactusMeshes[stage][variant])
                                 .material(cactusMaterials[stage][variant])
                                 .build();

    sceneObjects.push_back(cactusObj);
    plants.emplace_back(sceneObjects.size() - 1, PlantType::Cactus, stage,
                        variant);
  }

  for (int i = 0; i < config.numTrees; i++) {
    const float radius = radiusDist(rng);
    const float angle = angleDist(rng);
    const float x = radius * std::cos(angle);
    const float z = radius * std::sin(angle);

    const float y = getTerrainHeightAt(terrainMesh, x, z);
    const glm::vec3 terrainNormal = getTerrainNormalAt(terrainMesh, x, z);

    int stage = 0;
    if (config.randomGrowthStages) {
      std::uniform_int_distribution<int> stageDist(0, 7);
      stage = stageDist(rng);
    }

    const float baseYaw = rotationDist(rng);
    const float rotationVariance =
        varianceDist(rng) * config.rotationVariance * 15.0f;
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 rotationAxis =
        glm::normalize(glm::cross(up, terrainNormal));
    const float tiltAngle =
        std::acos(glm::clamp(glm::dot(up, terrainNormal), -1.0f, 1.0f));
    const float pitch =
        tiltAngle * config.rotationVariance * (180.0f / glm::pi<float>());
    const float roll = 0.0f;
    const float yaw = baseYaw + rotationVariance;

    const float baseScale = 1.0f;
    const float scaleVarianceX = varianceDist(rng) * config.scaleVariance;
    const float scaleVarianceY = varianceDist(rng) * config.scaleVariance;
    const float scaleVarianceZ = varianceDist(rng) * config.scaleVariance;
    const float scaleX = baseScale + scaleVarianceX;
    const float scaleY = baseScale + scaleVarianceY;
    const float scaleZ = baseScale + scaleVarianceZ;

    const Mesh* mesh = meshManager->getMesh(treeMeshes[stage]);
    const float yOffset = calculateMeshBottomOffset(mesh);
    const float scaledYOffset = yOffset * scaleY;
    const float sinkAmount = sinkDist(rng);

    const Object treeObj = ObjectBuilder()
                               .name("Tree_" + std::to_string(i))
                               .position(x, y - scaledYOffset - sinkAmount, z)
                               .rotationEuler(pitch, yaw, roll)
                               .scale(scaleX, scaleY, scaleZ)
                               .mesh(treeMeshes[stage])
                               .material(treeMaterials[stage])
                               .build();

    sceneObjects.push_back(treeObj);
    plants.emplace_back(sceneObjects.size() - 1, PlantType::Tree, stage, 0);
  }

  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Successfully spawned ",
             plants.size(), " plants");
}

float PlantManager::calculateMeshBottomOffset(const Mesh* mesh) const {
  if (!mesh || mesh->vertices.empty()) {
    return 0.0f;
  }

  float minY = std::numeric_limits<float>::max();

  for (const auto& vertex : mesh->vertices) {
    minY = std::min(minY, vertex.pos.y);
  }

  return minY;
}

void PlantManager::growPlant(std::vector<Object>& sceneObjects,
                             size_t plantIndex) {
  if (plantIndex >= plants.size()) {
    Debug::log(Debug::Category::PLANTMANAGER,
               "PlantManager: Invalid plant index: ", plantIndex);
    return;
  }

  Plant& plant = plants[plantIndex];
  Object& obj = sceneObjects[plant.getObjectIndex()];

  if (plant.getType() == PlantType::Cactus) {
    if (plant.getStage() < 2) {
      plant.setStage(plant.getStage() + 1);

      obj.setMesh(cactusMeshes[plant.getStage()][plant.getVariant()]);
      obj.setMaterial(cactusMaterials[plant.getStage()][plant.getVariant()]);

      Debug::log(Debug::Category::PLANTMANAGER,
                 "PlantManager: Cactus grew to stage ", plant.getStage());
    }
  } else if (plant.getType() == PlantType::Tree) {
    if (plant.getStage() < 7) {
      plant.setStage(plant.getStage() + 1);

      obj.setMesh(treeMeshes[plant.getStage()]);
      obj.setMaterial(treeMaterials[plant.getStage()]);

      Debug::log(Debug::Category::PLANTMANAGER,
                 "PlantManager: Tree grew to stage ", plant.getStage());
    }
  }
}