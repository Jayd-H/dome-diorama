#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Application.h"
#include "Particles/DustEmitter.h"
#include "Particles/FireEmitter.h"
#include "Particles/RainEmitter.h"
#include "Particles/SmokeEmitter.h"
#include "Resources/Object.h"
#include "Scene/WorldState.h"
#include "Util/ConfigParser.h"
#include "Util/Debug.h"

std::vector<Object> createScene(const ConfigParser& config,
                                MeshManager* meshManager,
                                MaterialManager* materialManager,
                                PlantManager* plantManager,
                                ParticleManager* particleManager,
                                LightManager* lightManager,
                                LightID& sunLightID) {
  Debug::log(Debug::Category::MAIN, "Creating materials and scene...");

  std::vector<Object> sceneObjects;

  const MaterialID sunMaterialID =
      materialManager->registerMaterial(MaterialBuilder()
                                            .name("Sun Material")
                                            .albedoColor(1.0f, 0.9f, 0.6f)
                                            .emissiveIntensity(1.0f)
                                            .roughness(1.0f)
                                            .metallic(0.0f));

  const MaterialID sandMaterialID = materialManager->registerMaterial(
      MaterialBuilder()
          .name("Sand Material")
          .albedoMap("./Models/textures/rockytrail/rocky_trail_02_diff_4k.jpg")
          .normalMap("./Models/textures/rockytrail/rocky_trail_02_ao_4k.jpg")
          .roughnessMap(
              "./Models/textures/rockytrail/rocky_trail_02_arm_4k.jpg")
          .heightMap("./Models/textures/rockytrail/rocky_trail_02_disp_4k.jpg")
          .heightScale(1.0f)
          .textureScale(50.0f));

  const MeshID sphereMesh = meshManager->createSphere(10.0f, 32);

  const MeshID sandTerrainMesh = meshManager->createProceduralTerrain(
      config.getFloat("Terrain.terrain_size", 300.0f),
      config.getInt("Terrain.terrain_resolution", 100),
      config.getFloat("Terrain.terrain_height", 10.0f),
      config.getFloat("Terrain.terrain_frequency", 2.0f),
      config.getInt("Terrain.terrain_octaves", 2),
      config.getFloat("Terrain.terrain_persistence", 0.6f),
      config.getInt("Terrain.terrain_seed", 42));

  const Object sun = ObjectBuilder()
                         .name("Sun")
                         .position(0.0f, 0.0f, 0.0f)
                         .mesh(sphereMesh)
                         .material(sunMaterialID)
                         .scale(1.0f)
                         .build();

  const Object sandPlane = ObjectBuilder()
                               .name("Sand Terrain")
                               .position(0.0f, 0.0f, 0.0f)
                               .mesh(sandTerrainMesh)
                               .material(sandMaterialID)
                               .build();

  sceneObjects.push_back(sun);
  sceneObjects.push_back(sandPlane);

  const MeshID skyboxSphereMesh = meshManager->createSphere(100.0f, 64);

  const MaterialID skyboxMaterialID = materialManager->registerMaterial(
      MaterialBuilder()
          .name("Skybox Sphere Material")
          .albedoColor(238.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f)
          .metallic(0.0f)
          .roughness(0.1f)
          .transparent(true)
          .opacity(0.1f));

  const Object skyboxSphere = ObjectBuilder()
                                  .name("Skybox Sphere")
                                  .position(0.0f, 0.0f, 0.0f)
                                  .mesh(skyboxSphereMesh)
                                  .material(skyboxMaterialID)
                                  .scale(3.0f)
                                  .build();

  sceneObjects.push_back(skyboxSphere);

  const MeshID pokeballWhiteMesh =
      meshManager->loadFromOBJ("./Models/PokeWhite.obj");
  const MaterialID pokeballWhiteMaterial =
      materialManager->loadFromMTL("./Models/PokeWhite.mtl");

  const MeshID pokeballBlackMesh =
      meshManager->loadFromOBJ("./Models/PokeBlack.obj");
  const MaterialID pokeballBlackMaterial =
      materialManager->loadFromMTL("./Models/PokeBlack.mtl");

  const Object pokeballWhite = ObjectBuilder()
                                   .name("Pokeball White")
                                   .position(-10.0f, -130.0f, 0.0f)
                                   .mesh(pokeballWhiteMesh)
                                   .material(pokeballWhiteMaterial)
                                   .scale(3.05f)
                                   .build();

  const Object pokeballBlack = ObjectBuilder()
                                   .name("Pokeball Black")
                                   .position(0.0f, 0.0f, 0.0f)
                                   .mesh(pokeballBlackMesh)
                                   .material(pokeballBlackMaterial)
                                   .scale(3.05f)
                                   .build();

  sceneObjects.push_back(pokeballWhite);
  sceneObjects.push_back(pokeballBlack);

  const Mesh* const terrainMesh = meshManager->getMesh(sandTerrainMesh);

  PlantSpawnConfig plantConfig;
  plantConfig.numCacti = config.getInt("Plants.num_cacti", 400);
  plantConfig.numTrees = config.getInt("Plants.num_trees", 100);
  plantConfig.minRadius = config.getFloat("Plants.min_spawn_radius", 8.0f);
  plantConfig.maxRadius = config.getFloat("Plants.max_spawn_radius", 300.0f);
  plantConfig.seed = config.getInt("Plants.spawn_seed", 67);
  plantConfig.randomGrowthStages =
      config.getBool("Plants.random_growth_stages", true);
  plantConfig.scaleVariance = config.getFloat("Plants.scale_variance", 0.7f);
  plantConfig.rotationVariance =
      config.getFloat("Plants.rotation_variance", 0.8f);

  plantManager->spawnPlantsOnTerrain(sceneObjects, terrainMesh, plantConfig);

  const Light sunLight = LightBuilder()
                             .type(LightType::Sun)
                             .name("Sun Light")
                             .direction(0.0f, -1.0f, 0.0f)
                             .color(1.0f, 0.95f, 0.8f)
                             .intensity(5.0f)
                             .castsShadows(true)
                             .build();

  sunLightID = lightManager->addLight(sunLight);

  const MaterialID particleMaterialID =
      materialManager->registerMaterial(MaterialBuilder()
                                            .name("Particle Material")
                                            .albedoColor(1.0f, 1.0f, 1.0f)
                                            .roughness(0.0f)
                                            .metallic(0.0f)
                                            .transparent(true));

  FireEmitter* const fireEmitter = FireEmitterBuilder()
                                       .name("Fire Emitter")
                                       .position(-5.0f, 0.5f, 0.0f)
                                       .maxParticles(800)
                                       .particleLifetime(2.0f)
                                       .material(particleMaterialID)
                                       .baseColor(1.0f, 0.9f, 0.1f)
                                       .tipColor(1.0f, 0.3f, 0.0f)
                                       .upwardSpeed(2.0f)
                                       .spawnRadius(0.2f)
                                       .particleScale(0.5f)
                                       .build();

  SmokeEmitter* const smokeEmitter = SmokeEmitterBuilder()
                                         .name("Smoke Emitter")
                                         .position(5.0f, 0.5f, 0.0f)
                                         .maxParticles(600)
                                         .particleLifetime(3.0f)
                                         .material(particleMaterialID)
                                         .baseColor(0.3f, 0.3f, 0.3f)
                                         .tipColor(0.6f, 0.6f, 0.6f)
                                         .upwardSpeed(1.5f)
                                         .horizontalSpread(0.8f)
                                         .spawnRadius(0.3f)
                                         .particleScale(1.5f)
                                         .build();

  RainEmitter* const rainEmitter = RainEmitterBuilder()
                                       .name("Rain Emitter")
                                       .position(0.0f, 30.0f, 10.0f)
                                       .maxParticles(1000)
                                       .particleLifetime(2.0f)
                                       .material(particleMaterialID)
                                       .baseColor(0.6f, 0.7f, 0.9f)
                                       .tipColor(0.8f, 0.9f, 1.0f)
                                       .downwardSpeed(15.0f)
                                       .windStrength(2.0f)
                                       .spawnRadius(15.0f)
                                       .particleScale(0.1f)
                                       .build();

  DustEmitter* const dustEmitter = DustEmitterBuilder()
                                       .name("Dust Emitter")
                                       .position(0.0f, 5.0f, -10.0f)
                                       .maxParticles(500)
                                       .particleLifetime(4.0f)
                                       .material(particleMaterialID)
                                       .baseColor(0.7f, 0.6f, 0.5f)
                                       .tipColor(0.5f, 0.4f, 0.3f)
                                       .swirlingSpeed(1.0f)
                                       .driftSpeed(0.5f)
                                       .spawnRadius(5.0f)
                                       .particleScale(0.3f)
                                       .build();

  particleManager->registerEmitter(fireEmitter);
  particleManager->registerEmitter(smokeEmitter);
  particleManager->registerEmitter(rainEmitter);
  particleManager->registerEmitter(dustEmitter);

  Debug::log(Debug::Category::MAIN, "Created 4 particle emitters");
  Debug::log(Debug::Category::MAIN, "Created ", sceneObjects.size(),
             " scene objects and ", lightManager->getLightCount(), " lights");
  Debug::log(Debug::Category::MAIN, "Spawned ",
             plantManager->getPlants().size(), " plants");

  return sceneObjects;
}

int main() {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  ConfigParser config;
  if (!config.load("config.ini")) {
    Debug::log(Debug::Category::MAIN,
               "Warning: Could not load config.ini, using defaults");
  }

  Debug::setEnabled(Debug::Category::MAIN,
                    config.getBool("Debug.debug_main", true));
  Debug::setEnabled(Debug::Category::CAMERA,
                    config.getBool("Debug.debug_camera", false));
  Debug::setEnabled(Debug::Category::INPUT,
                    config.getBool("Debug.debug_input", false));
  Debug::setEnabled(Debug::Category::RENDERING,
                    config.getBool("Debug.debug_rendering", true));
  Debug::setEnabled(Debug::Category::VULKAN,
                    config.getBool("Debug.debug_vulkan", true));
  Debug::setEnabled(Debug::Category::SKYBOX,
                    config.getBool("Debug.debug_skybox", true));
  Debug::setEnabled(Debug::Category::PLANTMANAGER,
                    config.getBool("Debug.debug_plantmanager", true));
  Debug::setEnabled(Debug::Category::WORLD,
                    config.getBool("Debug.debug_world", true));
  Debug::setEnabled(Debug::Category::PARTICLES,
                    config.getBool("Debug.debug_particles", true));
  Debug::setEnabled(Debug::Category::MESH,
                    config.getBool("Debug.debug_mesh", false));
  Debug::setEnabled(Debug::Category::LIGHTS,
                    config.getBool("Debug.debug_lights", true));
  Debug::setEnabled(Debug::Category::SCENE,
                    config.getBool("Debug.debug_scene", true));
  Debug::setEnabled(Debug::Category::OBJECTS,
                    config.getBool("Debug.debug_objects", false));
  Debug::setEnabled(Debug::Category::TEXTURE,
                    config.getBool("Debug.debug_texture", false));
  Debug::setEnabled(Debug::Category::MATERIALS,
                    config.getBool("Debug.debug_materials", false));
  Debug::setEnabled(Debug::Category::POSTPROCESSING,
                    config.getBool("Debug.debug_postprocessing", true));
  Debug::setEnabled(Debug::Category::SHADOWS,
                    config.getBool("Debug.debug_shadows", true));

  try {
    Application app;

    Debug::log(Debug::Category::MAIN, "Initializing Vulkan...");
    app.init();

    Debug::log(Debug::Category::MAIN, "Creating scene...");
    LightID sunLightID = INVALID_LIGHT_ID;
    std::vector<Object> const sceneObjects =
        createScene(config, app.getMeshManager(), app.getMaterialManager(),
                    app.getPlantManager(), app.getParticleManager(),
                    app.getLightManager(), sunLightID);

    app.setSunLightID(sunLightID);
    app.setScene(sceneObjects);

    WorldConfig worldConfig;
    worldConfig.dayLengthInSeconds =
        config.getFloat("World.day_length", 120.0f);
    worldConfig.startingHour = config.getInt("World.starting_hour", 12);
    worldConfig.startingMinute = config.getInt("World.starting_minute", 0);
    worldConfig.startingTemperature =
        config.getFloat("World.starting_temperature", 20.0f);
    worldConfig.minTemperature =
        config.getFloat("World.min_temperature", -15.0f);
    worldConfig.maxTemperature =
        config.getFloat("World.max_temperature", 40.0f);
    worldConfig.startingHumidity =
        config.getFloat("World.starting_humidity", 0.3f);
    worldConfig.minHumidity = config.getFloat("World.min_humidity", 0.1f);
    worldConfig.maxHumidity = config.getFloat("World.max_humidity", 0.95f);
    worldConfig.startingWindSpeed =
        config.getFloat("World.starting_wind_speed", 2.0f);
    worldConfig.minWindSpeed = config.getFloat("World.min_wind_speed", 0.5f);
    worldConfig.maxWindSpeed = config.getFloat("World.max_wind_speed", 10.0f);
    worldConfig.parameterUpdateInterval =
        config.getFloat("World.parameter_update_interval", 30.0f);
    worldConfig.dayNightTempVariation =
        config.getFloat("World.day_night_temp_variation", 8.0f);

    app.setWorldConfig(worldConfig);

    Debug::log(Debug::Category::MAIN, "Scene created, starting main loop...");
    app.run();

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}