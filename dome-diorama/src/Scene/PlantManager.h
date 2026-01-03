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

class PlantManager;

class Plant final {
 public:
  Plant(size_t objIndex, PlantType t, int s, int v)
      : state_(),
        objectIndex_(objIndex),
        type_(t),
        stage_(s),
        variant_(v),
        burnRevertTime_(0.0f) {}

  size_t getObjectIndex() const { return objectIndex_; }
  PlantType getType() const { return type_; }
  int getStage() const { return stage_; }
  int getVariant() const { return variant_; }

  const PlantState& getState() const { return state_; }

  void setStage(int s) { stage_ = s; }
  void setVariant(int v) { variant_ = v; }

  int getMaxStage() const { return type_ == PlantType::Cactus ? 2 : 7; }

  friend class PlantManager;

 private:
  PlantState state_;
  size_t objectIndex_;
  PlantType type_;
  int stage_;
  int variant_;
  float burnRevertTime_;
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
  glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
  float temperature = 20.0f;
  float humidity = 0.5f;
  float precipitationIntensity = 0.0f;
  float windStrength = 1.0f;
  float deltaTime = 0.016f;
};

struct PlantTransformData {
  float x, y, z;
  float pitch, yaw, roll;
  float scaleX, scaleY, scaleZ;
  float sinkAmount;
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

  const std::vector<Plant>& getPlants() const { return plants; }

  const PlantWindData& getWindData() const { return windData; }

  const std::vector<size_t>& getPlantObjectIndices() const {
    return plantObjectIndices;
  }

  void setTerrainMesh(const Mesh* mesh) { terrainMesh = mesh; }

 private:
  std::mt19937 rng;
  std::vector<std::vector<MeshID>> cactusMeshes;
  std::vector<std::vector<MaterialID>> cactusMaterials;
  std::vector<MeshID> treeMeshes;
  std::vector<MaterialID> treeMaterials;
  std::vector<Plant> plants;
  std::vector<size_t> plantObjectIndices;
  PlantWindData windData;

  MeshManager* meshManager;
  MaterialManager* materialManager;
  ParticleManager* particleManager = nullptr;
  const Mesh* terrainMesh = nullptr;

  float totalTime = 0.0f;
  MaterialID fireMaterialID = INVALID_MATERIAL_ID;

  void loadCactiModels();
  void loadTreeModels();
  float getTerrainHeightAt(const Mesh* terrainMesh, float x, float z) const;
  glm::vec3 getTerrainNormalAt(const Mesh* terrainMesh, float x, float z) const;
  float calculateMeshBottomOffset(const Mesh* mesh) const;

  PlantTransformData calculatePlantTransform(const Mesh* terrainMesh,
                                             const PlantSpawnConfig& config,
                                             float radius, float angle);
  void updatePlantVisuals(Object& obj, const Plant& plant);

  void updatePlantHealth(Plant& plant,
                         const EnvironmentConditions& conditions) const;
  void updatePlantGrowth(Plant& plant, std::vector<Object>& sceneObjects,
                         size_t plantIndex,
                         const EnvironmentConditions& conditions);
  void updatePlantFire(Plant& plant, std::vector<Object>& sceneObjects,
                       size_t plantIndex,
                       const EnvironmentConditions& conditions);
  void updatePlantSpreading(Plant& plant, std::vector<Object>& sceneObjects,
                            size_t plantIndex,
                            const EnvironmentConditions& conditions);
  void checkFireSpread(std::vector<Object>& sceneObjects);
  void startFire(Plant& plant, const glm::vec3& position);
  void extinguishFire(Plant& plant);
  void killPlant(Plant& plant, std::vector<Object>& sceneObjects);
  void spawnOffspring(std::vector<Object>& sceneObjects, const Plant& parent,
                      const glm::vec3& parentPos);
};