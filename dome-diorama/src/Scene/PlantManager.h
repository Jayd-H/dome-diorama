#pragma once
#include <random>
#include <vector>

#include "Particles/ParticleEmitter.h"
#include "PlantState.h"
#include "Resources/MaterialManager.h"
#include "Resources/MeshManager.h"
#include "Resources/Object.h"

class ParticleManager;

enum class PlantType { Cactus, Tree };

struct PlantTypeData {
  std::vector<MeshID> stageMeshes;
  std::vector<MaterialID> stageMaterials;
};

class Plant final {
 public:
  Plant(size_t objIndex, PlantType t, int s, int v)
      : objectIndex_(objIndex), type_(t), stage_(s), variant_(v), state_() {}

  size_t getObjectIndex() const { return objectIndex_; }
  PlantType getType() const { return type_; }
  int getStage() const { return stage_; }
  int getVariant() const { return variant_; }
  PlantState& getState() { return state_; }
  const PlantState& getState() const { return state_; }

  void setStage(int s) { stage_ = s; }
  void setVariant(int v) { variant_ = v; }

  int getMaxStage() const { return type_ == PlantType::Cactus ? 2 : 7; }

 private:
  size_t objectIndex_;
  PlantType type_;
  int stage_;
  int variant_;
  PlantState state_;
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

struct EnvironmentConditions {
  float temperature = 20.0f;
  float humidity = 0.5f;
  float precipitationIntensity = 0.0f;
  glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
  float windStrength = 1.0f;
  float deltaTime = 0.016f;
};

class PlantManager final {
 public:
  PlantManager(MeshManager* meshMgr, MaterialManager* materialMgr);
  ~PlantManager();

  PlantManager(const PlantManager&) = delete;
  PlantManager& operator=(const PlantManager&) = delete;

  void init();
  void setParticleManager(ParticleManager* pm) { particleManager = pm; }
  void setFireMaterialID(MaterialID id) { fireMaterialID = id; }

  void spawnPlantsOnTerrain(std::vector<Object>& sceneObjects,
                            const Mesh* terrainMesh,
                            const PlantSpawnConfig& config);
  void growPlant(std::vector<Object>& sceneObjects, size_t plantIndex);
  void shrinkPlant(std::vector<Object>& sceneObjects, size_t plantIndex);

  void updateEnvironment(std::vector<Object>& sceneObjects,
                         const EnvironmentConditions& conditions);

  std::vector<Plant>& getPlants() { return plants; }
  const std::vector<Plant>& getPlants() const { return plants; }

  const PlantWindData& getWindData() const { return windData; }

  const std::vector<size_t>& getPlantObjectIndices() const {
    return plantObjectIndices;
  }

 private:
  MeshManager* meshManager;
  MaterialManager* materialManager;
  ParticleManager* particleManager = nullptr;
  MaterialID fireMaterialID = INVALID_MATERIAL_ID;

  std::mt19937 rng;
  std::vector<Plant> plants;
  std::vector<size_t> plantObjectIndices;
  std::vector<std::vector<MeshID>> cactusMeshes;
  std::vector<std::vector<MaterialID>> cactusMaterials;
  std::vector<MeshID> treeMeshes;
  std::vector<MaterialID> treeMaterials;

  PlantWindData windData;
  float totalTime = 0.0f;

  void loadCactiModels();
  void loadTreeModels();
  float getTerrainHeightAt(const Mesh* terrainMesh, float x, float z) const;
  glm::vec3 getTerrainNormalAt(const Mesh* terrainMesh, float x, float z) const;
  float calculateMeshBottomOffset(const Mesh* mesh) const;

  void updatePlantHealth(Plant& plant, const EnvironmentConditions& conditions);
  void updatePlantGrowth(Plant& plant, std::vector<Object>& sceneObjects,
                         size_t plantIndex,
                         const EnvironmentConditions& conditions);
  void updatePlantFire(Plant& plant, std::vector<Object>& sceneObjects,
                       size_t plantIndex,
                       const EnvironmentConditions& conditions);
  void checkFireSpread(std::vector<Object>& sceneObjects);
  void startFire(Plant& plant, const glm::vec3& position);
  void extinguishFire(Plant& plant);
  void killPlant(Plant& plant, std::vector<Object>& sceneObjects);
};