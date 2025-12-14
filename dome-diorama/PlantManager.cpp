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

  std::random_device rd;
  rng.seed(rd());
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

  MeshID mesh1 = meshManager->loadFromOBJ("./Models/Cacti/cacti2.obj");
  MaterialID mat1 = materialManager->loadFromMTL("./Models/Cacti/cacti2.mtl");
  cactusMeshes[0].push_back(mesh1);
  cactusMaterials[0].push_back(mat1);

  MeshID mesh2 = meshManager->loadFromOBJ("./Models/Cacti/cacti2v2.obj");
  MaterialID mat2 = materialManager->loadFromMTL("./Models/Cacti/cacti2v2.mtl");
  cactusMeshes[0].push_back(mesh2);
  cactusMaterials[0].push_back(mat2);

  MeshID mesh3 = meshManager->loadFromOBJ("./Models/Cacti/cacti3v1.obj");
  MaterialID mat3 = materialManager->loadFromMTL("./Models/Cacti/cacti3v1.mtl");
  cactusMeshes[1].push_back(mesh3);
  cactusMaterials[1].push_back(mat3);

  MeshID mesh4 = meshManager->loadFromOBJ("./Models/Cacti/cacti3v2.obj");
  MaterialID mat4 = materialManager->loadFromMTL("./Models/Cacti/cacti3v2.mtl");
  cactusMeshes[1].push_back(mesh4);
  cactusMaterials[1].push_back(mat4);

  MeshID mesh5 = meshManager->loadFromOBJ("./Models/Cacti/cacti3v3.obj");
  MaterialID mat5 = materialManager->loadFromMTL("./Models/Cacti/cacti3v3.mtl");
  cactusMeshes[1].push_back(mesh5);
  cactusMaterials[1].push_back(mat5);

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

void PlantManager::spawnPlantsOnTerrain(std::vector<Object>& sceneObjects,
                                        const Mesh* terrainMesh, int numCacti,
                                        int numTrees) {
  Debug::log(Debug::Category::RENDERING, "PlantManager: Spawning ", numCacti,
             " cacti and ", numTrees, " trees");

  std::uniform_real_distribution<float> radiusDist(10.0f, 90.0f);
  std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
  std::uniform_real_distribution<float> rotationDist(0.0f, 360.0f);

  for (int i = 0; i < numCacti; i++) {
    float radius = radiusDist(rng);
    float angle = angleDist(rng);
    float x = radius * std::cos(angle);
    float z = radius * std::sin(angle);

    float y = getTerrainHeightAt(terrainMesh, x, z);

    std::uniform_int_distribution<int> variantDist(
        0, static_cast<int>(cactusMeshes[0].size()) - 1);
    int variant = variantDist(rng);

    Object cactusObj = ObjectBuilder()
                           .name("Cactus_" + std::to_string(i))
                           .position(x, y, z)
                           .rotationEuler(0.0f, rotationDist(rng), 0.0f)
                           .mesh(cactusMeshes[0][variant])
                           .material(cactusMaterials[0][variant])
                           .build();

    sceneObjects.push_back(cactusObj);

    plants.emplace_back(sceneObjects.size() - 1, PlantType::Cactus, 0, variant);
  }

  for (int i = 0; i < numTrees; i++) {
    float radius = radiusDist(rng);
    float angle = angleDist(rng);
    float x = radius * std::cos(angle);
    float z = radius * std::sin(angle);

    float y = getTerrainHeightAt(terrainMesh, x, z);

    Object treeObj = ObjectBuilder()
                         .name("Tree_" + std::to_string(i))
                         .position(x, y, z)
                         .rotationEuler(0.0f, rotationDist(rng), 0.0f)
                         .mesh(treeMeshes[0])
                         .material(treeMaterials[0])
                         .build();

    sceneObjects.push_back(treeObj);

    plants.emplace_back(sceneObjects.size() - 1, PlantType::Tree, 0, 0);
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