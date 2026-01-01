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

class Plant final {
 public:
  Plant(size_t objIndex, PlantType t, int s, int v)
      : objectIndex_(objIndex), type_(t), stage_(s), variant_(v) {}

  size_t getObjectIndex() const { return objectIndex_; }
  PlantType getType() const { return type_; }
  int getStage() const { return stage_; }
  int getVariant() const { return variant_; }

  void setStage(int s) { stage_ = s; }
  void setVariant(int v) { variant_ = v; }

 private:
  size_t objectIndex_;
  PlantType type_;
  int stage_;
  int variant_;
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

class PlantManager final {
 public:
  PlantManager(MeshManager* meshMgr, MaterialManager* materialMgr);
  ~PlantManager();

  PlantManager(const PlantManager&) = delete;
  PlantManager& operator=(const PlantManager&) = delete;

  void init();
  void spawnPlantsOnTerrain(std::vector<Object>& sceneObjects,
                            const Mesh* terrainMesh,
                            const PlantSpawnConfig& config);
  void growPlant(std::vector<Object>& sceneObjects, size_t plantIndex);

  std::vector<Plant> getPlants() const { return plants; }

 private:
  MeshManager* meshManager;
  MaterialManager* materialManager;
  std::mt19937 rng;
  std::vector<Plant> plants;
  std::vector<std::vector<MeshID>> cactusMeshes;
  std::vector<std::vector<MaterialID>> cactusMaterials;
  std::vector<MeshID> treeMeshes;
  std::vector<MaterialID> treeMaterials;

  void loadCactiModels();
  void loadTreeModels();
  float getTerrainHeightAt(const Mesh* terrainMesh, float x, float z) const;
  glm::vec3 getTerrainNormalAt(const Mesh* terrainMesh, float x, float z) const;
  float calculateMeshBottomOffset(const Mesh* mesh) const;
};