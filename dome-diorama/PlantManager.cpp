#include "PlantManager.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Debug.h"

PlantManager::PlantManager(MeshManager* meshManager,
                           MaterialManager* materialManager)
    : meshManager(meshManager), materialManager(materialManager) {
  Debug::log(Debug::Category::RENDERING, "PlantManager: Constructor called");
}

PlantManager::~PlantManager() {
  Debug::log(Debug::Category::RENDERING, "PlantManager: Destructor called");
}

void PlantManager::init() {
  Debug::log(Debug::Category::RENDERING, "PlantManager: Initializing");

  loadCactiModels();
  loadTreeModels();

  Debug::log(Debug::Category::RENDERING,
             "PlantManager: Initialization complete");
}

void PlantManager::loadCactiModels() {
  Debug::log(Debug::Category::RENDERING, "PlantManager: Loading cactus models");

  struct CactusModelPair {
    std::string objPath;
    std::string mtlPath;
  };

  std::vector<CactusModelPair> stage0Models = {
      {"./Models/Cacti/cacti2.obj", "./Models/Cacti/cacti2.mtl"},
      {"./Models/Cacti/cacti2v2.obj", "./Models/Cacti/cacti2v2.mtl"}};

  std::vector<CactusModelPair> stage1Models = {
      {"./Models/Cacti/cacti3v1.obj", "./Models/Cacti/cacti3v1.mtl"},
      {"./Models/Cacti/cacti3v2.obj", "./Models/Cacti/cacti3v2.mtl"},
      {"./Models/Cacti/cacti3v3.obj", "./Models/Cacti/cacti3v3.mtl"}};

  for (const auto& model : stage0Models) {
    MeshID meshID = meshManager->loadFromOBJ(model.objPath);
    MaterialID matID = materialManager->loadFromMTL(model.mtlPath);
    cactusMeshes[0].push_back(meshID);
    cactusMaterials[0].push_back(matID);
  }

  for (const auto& model : stage1Models) {
    MeshID meshID = meshManager->loadFromOBJ(model.objPath);
    MaterialID matID = materialManager->loadFromMTL(model.mtlPath);
    cactusMeshes[1].push_back(meshID);
    cactusMaterials[1].push_back(matID);
  }

  cactusMeshes[2] = cactusMeshes[1];
  cactusMaterials[2] = cactusMaterials[1];

  Debug::log(
      Debug::Category::RENDERING,
      "PlantManager: Loaded cactus models - Stage 0: ", cactusMeshes[0].size(),
      " variants, Stage 1: ", cactusMeshes[1].size(),
      " variants, Stage 2: ", cactusMeshes[2].size(), " variants");
}

void PlantManager::loadTreeModels() {
  Debug::log(Debug::Category::RENDERING, "PlantManager: Loading tree models");

  for (int i = 1; i <= 8; i++) {
    std::string objPath = "./Models/Trees/jt" + std::to_string(i) + ".obj";
    std::string mtlPath = "./Models/Trees/jt" + std::to_string(i) + ".mtl";

    MeshID meshID = meshManager->loadFromOBJ(objPath);
    MaterialID matID = materialManager->loadFromMTL(mtlPath);

    treeMeshes.push_back(meshID);
    treeMaterials.push_back(matID);
  }

  Debug::log(Debug::Category::RENDERING, "PlantManager: Loaded ",
             treeMeshes.size(), " tree growth stages");
}

float PlantManager::getTerrainHeightAt(const Mesh* terrainMesh, float x,
                                       float z) {
  float closestDistSq = std::numeric_limits<float>::max();
  float height = 0.0f;

  for (const auto& vertex : terrainMesh->vertices) {
    float dx = vertex.pos.x - x;
    float dz = vertex.pos.z - z;
    float distSq = dx * dx + dz * dz;

    if (distSq < closestDistSq) {
      closestDistSq = distSq;
      height = vertex.pos.y;
    }
  }

  return height;
}

glm::vec3 PlantManager::getTerrainNormalAt(const Mesh* terrainMesh, float x,
                                           float z) {
  float closestDistSq = std::numeric_limits<float>::max();
  glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);

  for (const auto& vertex : terrainMesh->vertices) {
    float dx = vertex.pos.x - x;
    float dz = vertex.pos.z - z;
    float distSq = dx * dx + dz * dz;

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
  Debug::log(Debug::Category::RENDERING, "PlantManager: Spawning ",
             config.numCacti, " cacti and ", config.numTrees,
             " trees with seed ", config.seed);

  rng.seed(config.seed);

  std::uniform_real_distribution<float> radiusDist(config.minRadius,
                                                   config.maxRadius);
  std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
  std::uniform_real_distribution<float> rotationDist(0.0f, 360.0f);
  std::uniform_real_distribution<float> varianceDist(-1.0f, 1.0f);

  for (int i = 0; i < config.numCacti; i++) {
    float radius = radiusDist(rng);
    float angle = angleDist(rng);
    float x = radius * std::cos(angle);
    float z = radius * std::sin(angle);

    float y = getTerrainHeightAt(terrainMesh, x, z);
    glm::vec3 terrainNormal = getTerrainNormalAt(terrainMesh, x, z);

    int stage = 0;
    if (config.randomGrowthStages) {
      std::uniform_int_distribution<int> stageDist(0, 2);
      stage = stageDist(rng);
    }

    std::uniform_int_distribution<int> variantDist(
        0, static_cast<int>(cactusMeshes[stage].size()) - 1);
    int variant = variantDist(rng);

    float baseYaw = rotationDist(rng);
    float rotationVariance =
        varianceDist(rng) * config.rotationVariance * 15.0f;

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 rotationAxis = glm::normalize(glm::cross(up, terrainNormal));
    float tiltAngle =
        std::acos(glm::clamp(glm::dot(up, terrainNormal), -1.0f, 1.0f));

    float pitch =
        tiltAngle * config.rotationVariance * (180.0f / glm::pi<float>());
    float roll = 0.0f;
    float yaw = baseYaw + rotationVariance;

    float baseScale = 1.0f;
    float scaleX = baseScale + varianceDist(rng) * config.scaleVariance;
    float scaleY = baseScale + varianceDist(rng) * config.scaleVariance;
    float scaleZ = baseScale + varianceDist(rng) * config.scaleVariance;

    Object cactusObj = ObjectBuilder()
                           .name("Cactus_" + std::to_string(i))
                           .position(x, y, z)
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
    float radius = radiusDist(rng);
    float angle = angleDist(rng);
    float x = radius * std::cos(angle);
    float z = radius * std::sin(angle);

    float y = getTerrainHeightAt(terrainMesh, x, z);
    glm::vec3 terrainNormal = getTerrainNormalAt(terrainMesh, x, z);

    int stage = 0;
    if (config.randomGrowthStages) {
      std::uniform_int_distribution<int> stageDist(0, 7);
      stage = stageDist(rng);
    }

    float baseYaw = rotationDist(rng);
    float rotationVariance =
        varianceDist(rng) * config.rotationVariance * 15.0f;

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 rotationAxis = glm::normalize(glm::cross(up, terrainNormal));
    float tiltAngle =
        std::acos(glm::clamp(glm::dot(up, terrainNormal), -1.0f, 1.0f));

    float pitch =
        tiltAngle * config.rotationVariance * (180.0f / glm::pi<float>());
    float roll = 0.0f;
    float yaw = baseYaw + rotationVariance;

    float baseScale = 1.0f;
    float scaleX = baseScale + varianceDist(rng) * config.scaleVariance;
    float scaleY = baseScale + varianceDist(rng) * config.scaleVariance;
    float scaleZ = baseScale + varianceDist(rng) * config.scaleVariance;

    Object treeObj = ObjectBuilder()
                         .name("Tree_" + std::to_string(i))
                         .position(x, y, z)
                         .rotationEuler(pitch, yaw, roll)
                         .scale(scaleX, scaleY, scaleZ)
                         .mesh(treeMeshes[stage])
                         .material(treeMaterials[stage])
                         .build();

    sceneObjects.push_back(treeObj);
    plants.emplace_back(sceneObjects.size() - 1, PlantType::Tree, stage, 0);
  }

  Debug::log(Debug::Category::RENDERING, "PlantManager: Successfully spawned ",
             plants.size(), " plants");
}

void PlantManager::growPlant(std::vector<Object>& sceneObjects,
                             size_t plantIndex) {
  if (plantIndex >= plants.size()) {
    Debug::log(Debug::Category::RENDERING,
               "PlantManager: Invalid plant index: ", plantIndex);
    return;
  }

  Plant& plant = plants[plantIndex];
  Object& obj = sceneObjects[plant.objectIndex];

  if (plant.type == PlantType::Cactus) {
    if (plant.stage < 2) {
      plant.stage++;

      obj.setMesh(cactusMeshes[plant.stage][plant.variant]);
      obj.setMaterial(cactusMaterials[plant.stage][plant.variant]);

      Debug::log(Debug::Category::RENDERING,
                 "PlantManager: Cactus grew to stage ", plant.stage);
    }
  } else if (plant.type == PlantType::Tree) {
    if (plant.stage < 7) {
      plant.stage++;

      obj.setMesh(treeMeshes[plant.stage]);
      obj.setMaterial(treeMaterials[plant.stage]);

      Debug::log(Debug::Category::RENDERING,
                 "PlantManager: Tree grew to stage ", plant.stage);
    }
  }
}