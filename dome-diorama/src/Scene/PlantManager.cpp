#include "PlantManager.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Particles/FireEmitter.h"
#include "Particles/ParticleManager.h"
#include "Util/Debug.h"

PlantManager::PlantManager(MeshManager* meshMgr, MaterialManager* materialMgr)
    : rng(),
      cactusMeshes(3),
      cactusMaterials(3),
      treeMeshes(),
      treeMaterials(),
      plants(),
      plantObjectIndices(),
      windData{},
      meshManager(meshMgr),
      materialManager(materialMgr),
      particleManager(nullptr),
      fireMaterialID(INVALID_MATERIAL_ID) {
  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Constructor called");

  windData.windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
  windData.windStrength = 1.0f;
  windData.time = 0.0f;
  windData.swayAmount = 0.08f;
  windData.swaySpeed = 1.5f;
}

PlantManager::~PlantManager() {
  try {
    Debug::log(Debug::Category::PLANTMANAGER,
               "PlantManager: Destructor called");
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
  Debug::log(Debug::Category::PLANTMANAGER,
             "PlantManager: Loading cactus models");

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
  Debug::log(Debug::Category::PLANTMANAGER,
             "PlantManager: Loading tree models");

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

PlantTransformData PlantManager::calculatePlantTransform(
    const Mesh* terrainMesh, const PlantSpawnConfig& config, float radius,
    float angle) {
  PlantTransformData data{};

  std::uniform_real_distribution<float> rotationDist(0.0f, 360.0f);
  std::uniform_real_distribution<float> varianceDist(-1.0f, 1.0f);
  std::uniform_real_distribution<float> sinkDist(0.2f, 0.5f);

  data.x = radius * std::cos(angle);
  data.z = radius * std::sin(angle);
  data.y = getTerrainHeightAt(terrainMesh, data.x, data.z);

  const glm::vec3 terrainNormal =
      getTerrainNormalAt(terrainMesh, data.x, data.z);

  const float baseYaw = rotationDist(rng);
  const float rotationVariance =
      varianceDist(rng) * config.rotationVariance * 15.0f;
  const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
  const float tiltAngle =
      std::acos(glm::clamp(glm::dot(up, terrainNormal), -1.0f, 1.0f));

  data.pitch =
      tiltAngle * config.rotationVariance * (180.0f / glm::pi<float>());
  data.yaw = baseYaw + rotationVariance;
  data.roll = 0.0f;

  const float baseScale = 1.0f;
  const float scaleVarianceX = varianceDist(rng) * config.scaleVariance;
  const float scaleVarianceY = varianceDist(rng) * config.scaleVariance;
  const float scaleVarianceZ = varianceDist(rng) * config.scaleVariance;

  data.scaleX = baseScale + scaleVarianceX;
  data.scaleY = baseScale + scaleVarianceY;
  data.scaleZ = baseScale + scaleVarianceZ;

  data.sinkAmount = sinkDist(rng);

  return data;
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

  for (int i = 0; i < config.numCacti; i++) {
    const float radius = radiusDist(rng);
    const float angle = angleDist(rng);
    const PlantTransformData trans =
        calculatePlantTransform(terrainMesh, config, radius, angle);

    int stage = 0;
    if (config.randomGrowthStages) {
      std::uniform_int_distribution<int> stageDist(0, 2);
      stage = stageDist(rng);
    }

    std::uniform_int_distribution<int> variantDist(
        0, static_cast<int>(cactusMeshes[stage].size()) - 1);
    const int variant = variantDist(rng);

    const Mesh* const mesh = meshManager->getMesh(cactusMeshes[stage][variant]);
    const float yOffset = calculateMeshBottomOffset(mesh);
    const float scaledYOffset = yOffset * trans.scaleY;

    const Object cactusObj =
        ObjectBuilder()
            .name("Cactus_" + std::to_string(i))
            .position(trans.x, trans.y - scaledYOffset - trans.sinkAmount,
                      trans.z)
            .rotationEuler(trans.pitch, trans.yaw, trans.roll)
            .scale(trans.scaleX, trans.scaleY, trans.scaleZ)
            .mesh(cactusMeshes[stage][variant])
            .material(cactusMaterials[stage][variant])
            .build();

    sceneObjects.push_back(cactusObj);
    plantObjectIndices.push_back(sceneObjects.size() - 1);
    plants.emplace_back(sceneObjects.size() - 1, PlantType::Cactus, stage,
                        variant);
  }

  for (int i = 0; i < config.numTrees; i++) {
    const float radius = radiusDist(rng);
    const float angle = angleDist(rng);
    const PlantTransformData trans =
        calculatePlantTransform(terrainMesh, config, radius, angle);

    int stage = 0;
    if (config.randomGrowthStages) {
      std::uniform_int_distribution<int> stageDist(0, 7);
      stage = stageDist(rng);
    }

    const Mesh* const mesh = meshManager->getMesh(treeMeshes[stage]);
    const float yOffset = calculateMeshBottomOffset(mesh);
    const float scaledYOffset = yOffset * trans.scaleY;

    const Object treeObj =
        ObjectBuilder()
            .name("Tree_" + std::to_string(i))
            .position(trans.x, trans.y - scaledYOffset - trans.sinkAmount,
                      trans.z)
            .rotationEuler(trans.pitch, trans.yaw, trans.roll)
            .scale(trans.scaleX, trans.scaleY, trans.scaleZ)
            .mesh(treeMeshes[stage])
            .material(treeMaterials[stage])
            .build();

    sceneObjects.push_back(treeObj);
    plantObjectIndices.push_back(sceneObjects.size() - 1);
    plants.emplace_back(sceneObjects.size() - 1, PlantType::Tree, stage, 0);
  }

  Debug::log(Debug::Category::PLANTMANAGER,
             "PlantManager: Successfully spawned ", plants.size(), " plants");
}

void PlantManager::updatePlantVisuals(Object& obj, const Plant& plant) {
  if (plant.getType() == PlantType::Cactus) {
    obj.setMesh(cactusMeshes[plant.getStage()][plant.getVariant()]);
    obj.setMaterial(cactusMaterials[plant.getStage()][plant.getVariant()]);
  } else {
    obj.setMesh(treeMeshes[plant.getStage()]);
    obj.setMaterial(treeMaterials[plant.getStage()]);
  }
}

void PlantManager::growPlant(std::vector<Object>& sceneObjects,
                             size_t plantIndex) {
  if (plantIndex >= plants.size()) {
    Debug::log(Debug::Category::PLANTMANAGER,
               "PlantManager: Invalid plant index: ", plantIndex);
    return;
  }

  Plant& plant = plants[plantIndex];

  if (plant.getState().isDead) return;

  Object& obj = sceneObjects[plant.getObjectIndex()];

  if (plant.getType() == PlantType::Cactus) {
    if (plant.getStage() < 2) {
      plant.setStage(plant.getStage() + 1);

      int variant = plant.getVariant();
      if (variant >= static_cast<int>(cactusMeshes[plant.getStage()].size())) {
        std::uniform_int_distribution<int> variantDist(
            0, static_cast<int>(cactusMeshes[plant.getStage()].size()) - 1);
        variant = variantDist(rng);
        plant.setVariant(variant);
      }

      updatePlantVisuals(obj, plant);
      Debug::log(Debug::Category::PLANTMANAGER,
                 "PlantManager: Cactus grew to stage ", plant.getStage());
    }
  } else if (plant.getType() == PlantType::Tree) {
    if (plant.getStage() < 7) {
      plant.setStage(plant.getStage() + 1);
      updatePlantVisuals(obj, plant);
      Debug::log(Debug::Category::PLANTMANAGER,
                 "PlantManager: Tree grew to stage ", plant.getStage());
    }
  }
}

void PlantManager::shrinkPlant(std::vector<Object>& sceneObjects,
                               size_t plantIndex) {
  if (plantIndex >= plants.size()) {
    return;
  }

  Plant& plant = plants[plantIndex];

  if (plant.getState().isDead) return;

  Object& obj = sceneObjects[plant.getObjectIndex()];

  if (plant.getStage() > 0) {
    plant.setStage(plant.getStage() - 1);

    if (plant.getType() == PlantType::Cactus) {
      int variant = plant.getVariant();
      if (variant >= static_cast<int>(cactusMeshes[plant.getStage()].size())) {
        std::uniform_int_distribution<int> variantDist(
            0, static_cast<int>(cactusMeshes[plant.getStage()].size()) - 1);
        variant = variantDist(rng);
        plant.setVariant(variant);
      }
    }

    updatePlantVisuals(obj, plant);
    Debug::log(Debug::Category::PLANTMANAGER,
               "PlantManager: Plant shrunk to stage ", plant.getStage());
  } else {
    Debug::log(Debug::Category::PLANTMANAGER,
               "PlantManager: Plant at stage 0, killing plant");
    killPlant(plant, sceneObjects);
  }
}

void PlantManager::updateEnvironment(std::vector<Object>& sceneObjects,
                                     const EnvironmentConditions& conditions) {
  totalTime += conditions.deltaTime;

  windData.windDirection = glm::normalize(conditions.windDirection);
  windData.windStrength = conditions.windStrength;
  windData.time = totalTime;
  windData.swayAmount = 0.08f;
  windData.swaySpeed = 1.5f + conditions.windStrength * 0.1f;

  for (size_t i = 0; i < plants.size(); ++i) {
    Plant& plant = plants[i];

    if (plant.getState().isDead) continue;

    updatePlantHealth(plant, conditions);
    updatePlantFire(plant, sceneObjects, i, conditions);
    updatePlantGrowth(plant, sceneObjects, i, conditions);

    if (plant.getState().health <= 0.0f && !plant.getState().isDead) {
      Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Plant ", i,
                 " health <= 0, starting death sequence");
      if (plant.getStage() > 0) {
        shrinkPlant(sceneObjects, i);
        plant.state_.health = 20.0f;
      } else {
        killPlant(plant, sceneObjects);
      }
    }
  }

  checkFireSpread(sceneObjects);
}

void PlantManager::updatePlantHealth(
    Plant& plant, const EnvironmentConditions& conditions) const {
  PlantState& state = plant.state_;

  if (state.isDead) return;

  if (conditions.precipitationIntensity > 0.1f) {
    state.waterLevel += PlantState::RAIN_WATER_RATE *
                        conditions.precipitationIntensity *
                        conditions.deltaTime;
    state.waterLevel = std::min(state.waterLevel, 100.0f);
  }

  float drainMultiplier = 1.0f;
  if (conditions.temperature > 30.0f) {
    drainMultiplier = 1.0f + (conditions.temperature - 30.0f) * 0.05f;
  }
  state.waterLevel -=
      PlantState::WATER_DRAIN_RATE * drainMultiplier * conditions.deltaTime;
  state.waterLevel = std::max(state.waterLevel, 0.0f);

  if (state.waterLevel < 10.0f) {
    state.health -= 2.0f * conditions.deltaTime;
  }

  if (conditions.temperature > PlantState::HEAT_DAMAGE_THRESHOLD &&
      !state.isOnFire) {
    const float heatDamage =
        (conditions.temperature - PlantState::HEAT_DAMAGE_THRESHOLD) * 0.5f *
        conditions.deltaTime;
    state.health -= heatDamage;
  }

  state.health = std::max(0.0f, std::min(PlantState::MAX_HEALTH, state.health));
}

void PlantManager::updatePlantGrowth(Plant& plant,
                                     std::vector<Object>& sceneObjects,
                                     size_t plantIndex,
                                     const EnvironmentConditions& conditions) {
  PlantState& state = plant.state_;

  if (state.isOnFire || state.isDead) return;

  float growthRate = 0.0f;

  if (conditions.temperature >= PlantState::OPTIMAL_TEMP_MIN &&
      conditions.temperature <= PlantState::OPTIMAL_TEMP_MAX) {
    growthRate = 1.0f;
  } else if (conditions.temperature < PlantState::OPTIMAL_TEMP_MIN) {
    const float coldFactor =
        (conditions.temperature - PlantState::OPTIMAL_TEMP_MIN) /
        PlantState::OPTIMAL_TEMP_MIN;
    growthRate = std::max(0.0f, 1.0f + coldFactor);
  } else {
    const float heatFactor =
        (conditions.temperature - PlantState::OPTIMAL_TEMP_MAX) / 20.0f;
    growthRate = std::max(0.0f, 1.0f - heatFactor);
  }

  if (state.waterLevel > 30.0f) {
    growthRate *= 1.0f + (state.waterLevel - 30.0f) * 0.02f;
  } else if (state.waterLevel > 10.0f) {
    growthRate *= state.waterLevel / 30.0f;
  } else {
    growthRate *= 0.1f;
  }

  if (conditions.precipitationIntensity > 0.3f) {
    growthRate *= 1.5f;
  }

  state.growthProgress += growthRate * conditions.deltaTime * 10.0f;

  if (state.growthProgress >= PlantState::GROWTH_THRESHOLD) {
    state.growthProgress = 0.0f;

    if (plant.getStage() < plant.getMaxStage()) {
      growPlant(sceneObjects, plantIndex);
    }
  }
}

void PlantManager::updatePlantFire(Plant& plant,
                                   std::vector<Object>& sceneObjects,
                                   size_t plantIndex,
                                   const EnvironmentConditions& conditions) {
  PlantState& state = plant.state_;

  if (state.isDead) return;

  if (!state.isOnFire) {
    if (conditions.temperature > 45.0f && state.waterLevel < 30.0f) {
      const float drynessFactor = 1.0f - (state.waterLevel / 30.0f);
      const float heatFactor = (conditions.temperature - 45.0f) / 20.0f;
      const float fireChance = drynessFactor * heatFactor * 0.1f;

      std::uniform_real_distribution<float> dist(0.0f, 1.0f);
      const float roll = dist(rng);

      if (roll < fireChance * conditions.deltaTime) {
        Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Plant ",
                   plantIndex, " caught fire! Temp: ", conditions.temperature,
                   ", Water: ", state.waterLevel, ", Chance was: ", fireChance,
                   ", Roll was: ", roll);
        const Object& obj = sceneObjects[plant.getObjectIndex()];
        startFire(plant, obj.getPosition());
      }
    }
    return;
  }

  if (conditions.precipitationIntensity > 0.5f) {
    const float extinguishChance = conditions.precipitationIntensity * 0.5f;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng) < extinguishChance * conditions.deltaTime) {
      Debug::log(Debug::Category::PLANTMANAGER,
                 "PlantManager: Fire extinguished by rain on plant ",
                 plantIndex);
      extinguishFire(plant);
      return;
    }
  }

  state.burnTimer += conditions.deltaTime;
  state.health -= PlantState::BURN_DAMAGE_PER_SECOND * conditions.deltaTime;

  if (state.burnTimer >= 3.0f) {
    state.burnTimer = 0.0f;
    Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Burning plant ",
               plantIndex, " shrinking due to fire");
    shrinkPlant(sceneObjects, plantIndex);
  }

  if (state.health <= 0.0f) {
    Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Burning plant ",
               plantIndex, " died from fire damage");
    extinguishFire(plant);
    killPlant(plant, sceneObjects);
  }
}

void PlantManager::checkFireSpread(std::vector<Object>& sceneObjects) {
  std::vector<size_t> plantsToIgnite;

  for (size_t i = 0; i < plants.size(); ++i) {
    const Plant& plant = plants[i];
    const PlantState& state = plant.state_;

    if (!state.isOnFire || state.isDead) continue;

    const Object& burningObj = sceneObjects[plant.getObjectIndex()];

    for (size_t j = 0; j < plants.size(); ++j) {
      if (i == j) continue;

      Plant& otherPlant = plants[j];
      const PlantState& otherState = otherPlant.state_;

      if (otherState.isOnFire || otherState.isDead) continue;

      const Object& otherObj = sceneObjects[otherPlant.getObjectIndex()];

      const float distance =
          glm::length(burningObj.getPosition() - otherObj.getPosition());

      if (distance < PlantState::FIRE_SPREAD_RADIUS) {
        const float spreadChance =
            (1.0f - distance / PlantState::FIRE_SPREAD_RADIUS) * 0.05f *
            (1.0f - otherState.waterLevel / 100.0f);

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(rng) < spreadChance) {
          plantsToIgnite.push_back(j);
        }
      }
    }
  }

  for (size_t idx : plantsToIgnite) {
    Plant& plant = plants[idx];
    if (!plant.state_.isOnFire) {
      const Object& obj = sceneObjects[plant.getObjectIndex()];
      Debug::log(Debug::Category::PLANTMANAGER,
                 "PlantManager: Fire spread to plant ", idx);
      startFire(plant, obj.getPosition());
    }
  }
}

void PlantManager::startFire(Plant& plant, const glm::vec3& position) {
  PlantState& state = plant.state_;

  if (state.isOnFire || state.isDead) return;

  state.isOnFire = true;
  state.burnTimer = 0.0f;

  Debug::log(Debug::Category::PLANTMANAGER,
             "PlantManager: Starting fire at position (", position.x, ", ",
             position.y, ", ", position.z, ")");

  if (particleManager && fireMaterialID != INVALID_MATERIAL_ID) {
    FireEmitter* const fireEmitter =
        FireEmitterBuilder()
            .name("PlantFire_" + std::to_string(plant.getObjectIndex()))
            .position(position + glm::vec3(0.0f, 1.0f, 0.0f))
            .maxParticles(200)
            .particleLifetime(1.5f)
            .material(fireMaterialID)
            .baseColor(1.0f, 0.8f, 0.2f)
            .tipColor(1.0f, 0.2f, 0.0f)
            .upwardSpeed(3.0f)
            .spawnRadius(0.5f)
            .particleScale(0.8f)
            .build();

    state.fireEmitterID = particleManager->registerEmitter(fireEmitter);

    Debug::log(
        Debug::Category::PLANTMANAGER,
        "PlantManager: Created fire emitter with ID: ", state.fireEmitterID);
  } else {
    Debug::log(Debug::Category::PLANTMANAGER,
               "PlantManager: WARNING - No particle manager or fire material, "
               "fire will be invisible");
  }
}

void PlantManager::extinguishFire(Plant& plant) {
  PlantState& state = plant.state_;

  if (!state.isOnFire) return;

  state.isOnFire = false;
  state.burnTimer = 0.0f;

  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Extinguishing fire");

  if (particleManager && state.fireEmitterID != INVALID_EMITTER_ID) {
    ParticleEmitter* const emitter =
        particleManager->getEmitter(state.fireEmitterID);
    if (emitter) {
      emitter->setActive(false);
    }
    state.fireEmitterID = INVALID_EMITTER_ID;
  }
}

void PlantManager::killPlant(Plant& plant, std::vector<Object>& sceneObjects) {
  PlantState& state = plant.state_;

  if (state.isDead) return;

  state.isDead = true;
  extinguishFire(plant);

  Object& obj = sceneObjects[plant.getObjectIndex()];
  obj.setVisible(false);

  Debug::log(Debug::Category::PLANTMANAGER, "PlantManager: Plant at index ",
             plant.getObjectIndex(), " died and was hidden");
}