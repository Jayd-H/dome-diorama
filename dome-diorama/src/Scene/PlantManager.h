#pragma once
#include <random>
#include <vector>

#include "Resources/MaterialManager.h"
#include "Resources/MeshManager.h"
#include "Resources/Object.h"

enum class PlantType { Cactus, Tree };

struct PlantTypeData {
  std::vector<MeshID> stageMeshes;
  std::vector<MaterialID> stageMaterials;
};

class Plant {
 public:
  size_t objectIndex;
  PlantType type;
  int stage;
  int variant;

  Plant(size_t objIndex, PlantType t, int s, int v)
      : objectIndex(objIndex), type(t), stage(s), variant(v) {}
};

struct PlantSpawnConfig {
  int numCacti = 150;
  int numTrees = 100;
  float minRadius = 10.0f;
  float maxRadius = 90.0f;
  unsigned int seed = 67;
  bool randomGrowthStages = true;
  float scaleVariance = 0.3f;
  float rotationVariance = 0.2f;
};

class PlantManager {
 public:
  PlantManager(MeshManager* meshManager, MaterialManager* materialManager);
  ~PlantManager();

  void init();
  void spawnPlantsOnTerrain(std::vector<Object>& sceneObjects,
                            const Mesh* terrainMesh,
                            const PlantSpawnConfig& config);
  void growPlant(std::vector<Object>& sceneObjects, size_t plantIndex);

  const std::vector<Plant>& getPlants() const { return plants; }

 private:
  MeshManager* meshManager;
  MaterialManager* materialManager;
  std::vector<Plant> plants;

  std::vector<MeshID> cactusMeshes[3];
  std::vector<MaterialID> cactusMaterials[3];

  std::vector<MeshID> treeMeshes;
  std::vector<MaterialID> treeMaterials;

  std::mt19937 rng;

  void loadCactiModels();
  void loadTreeModels();
  float getTerrainHeightAt(const Mesh* terrainMesh, float x, float z);
  glm::vec3 getTerrainNormalAt(const Mesh* terrainMesh, float x, float z);
  float calculateMeshBottomOffset(const Mesh* mesh);
};