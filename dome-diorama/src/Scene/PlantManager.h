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
      : state_{},
        burnRevertTime_(0.0f),
        objectIndex_(objIndex),
        type_(t),
        stage_(s),
        variant_(v) {}

  size_t getObjectIndex() const { return objectIndex_; }
  PlantType getType() const { return type_; }
  int getStage() const { return stage_; }
  int getVariant() const { return variant_; }

  bool isOnFire() const { return state_.isOnFire; }
  bool isDead() const { return state_.isDead; }

  float getHealth() const { return state_.health; }
  void setHealth(float h) { state_.health = h; }
  void addHealth(float delta) { state_.health += delta; }

  float getWaterLevel() const { return state_.waterLevel; }
  void setWaterLevel(float w) { state_.waterLevel = w; }

  float getBurnTimer() const { return state_.burnTimer; }
  void setBurnTimer(float t) { state_.burnTimer = t; }

  float getGrowthProgress() const { return state_.growthProgress; }
  void setGrowthProgress(float g) { state_.growthProgress = g; }

  float getSpreadTimer() const { return state_.spreadTimer; }
  void setSpreadTimer(float t) { state_.spreadTimer = t; }

  int getFireEmitterID() const { return state_.fireEmitterID; }
  void setFireEmitterID(int id) { state_.fireEmitterID = id; }

  [[nodiscard]] const PlantState& getState() const noexcept { return state_; }

  void setStage(int s) { stage_ = s; }
  void setVariant(int v) { variant_ = v; }

  int getMaxStage() const { return type_ == PlantType::Cactus ? 2 : 7; }

  float getBurnRevertTime() const { return burnRevertTime_; }
  void setBurnRevertTime(float t) { burnRevertTime_ = t; }

  void markDead() { state_.isDead = true; }
  void ignite() { state_.isOnFire = true; }
  void extinguish() { state_.isOnFire = false; }

 private:
  PlantState state_;
  float burnRevertTime_;
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

struct EnvironmentConditions {
  glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
  float temperature = 20.0f;
  float humidity = 0.5f;
  float precipitationIntensity = 0.0f;
  float windStrength = 1.0f;
  float deltaTime = 0.016f;
  float time = 0.0f;
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

  void spawnPlantsOnTerrain(std::vector<Object>& sceneObjects,
                            const Mesh* targetTerrain,
                            const PlantSpawnConfig& config);

  void growPlant(std::vector<Object>& sceneObjects, size_t plantIndex);
  void shrinkPlant(std::vector<Object>& sceneObjects, size_t plantIndex);

  void updateEnvironment(std::vector<Object>& sceneObjects,
                         const EnvironmentConditions& conditions);


  void startFire(Plant& plant, const glm::vec3& position);

  inline size_t getPlantCount() const { return plants.size(); }

  inline const Plant& getPlant(size_t index) const { return plants[index]; }

  inline Plant& getPlantMutable(size_t index) { return plants[index]; }

  inline size_t getPlantObjectIndex(size_t index) const {
    return plants[index].getObjectIndex();
  }

  [[nodiscard]] const PlantWindData& getWindData() const noexcept {
    return windData;
  }

  inline void setParticleManager(ParticleManager* pm) { particleManager = pm; }

  inline void setFireMaterialID(MaterialID id) { fireMaterialID = id; }

  inline void setTerrainMesh(const Mesh* mesh) { terrainMesh = mesh; }

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

  MaterialID fireMaterialID = INVALID_MATERIAL_ID;

  void loadCactiModels();
  void loadTreeModels();

  float getTerrainHeightAt(const Mesh* terrainData, float x, float z) const;
  glm::vec3 getTerrainNormalAt(const Mesh* terrainData, float x, float z) const;
  float calculateMeshBottomOffset(const Mesh* mesh) const;

  void calculatePlantTransform(const Mesh* terrainData,
                               const PlantSpawnConfig& config, float radius,
                               float angle, PlantTransformData& outData);

  void updatePlantVisuals(Object& obj, const Plant& plant);
  void ensureValidCactusVariant(Plant& plant);

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
  void extinguishFire(Plant& plant);
  void killPlant(Plant& plant, std::vector<Object>& sceneObjects);

  void spawnOffspring(std::vector<Object>& sceneObjects, const Plant& parent,
                      const glm::vec3& parentPos);
};