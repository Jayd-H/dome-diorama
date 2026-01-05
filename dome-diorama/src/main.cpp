#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Application.h"
#include "Particles/EmitterTypes.h"
#include "Particles/ParticleEmitter.h"
#include "Resources/Object.h"
#include "Scene/WorldState.h"
#include "Util/ConfigParser.h"
#include "Util/Debug.h"

void printControls() {
  std::cout << R"(
GENERAL CONTROLS
ESC - Exit | R - Reset

CAMERA PRESETS
F1 - Overview | F2 - Navigation | F3 - Close-up

CAMERA CONTROLS
Arrow Keys - Rotate | CTRL+Arrows - Pan | CTRL+PgUp/Dn - Pan Up/Down
Enter - Toggle Orbit/FPS | RMB - Orbit | Scroll - Zoom/Speed
W/A/S/D - Move | Space/Shift - Up/Down

EFFECTS & TIME
F4 - Particle Effect | ]/[ - Time Scale +/- | P - Pause

RENDERING CONTROLS
1-3 - Fill/Wire/Point | 4-5 - Nearest/Linear | L - Shade Mode

WEATHER CONTROLS
T/G - Temp | H/N - Humidity | U/J - Wind | Y - Cycle
)" << std::endl;
}

void createScene(const ConfigParser& config, MeshManager* meshManager,
                 MaterialManager* materialManager, PlantManager* plantManager,
                 ParticleManager* particleManager, LightManager* lightManager,
                 LightID& sunLightID, std::vector<Object>& sceneObjects) {
  Debug::log(Debug::Category::MAIN, "Creating materials and scene...");

  const MaterialID sunMat =
      materialManager->registerMaterial(MaterialBuilder()
                                            .name("Sun")
                                            .albedoColor(1.0f, 0.9f, 0.6f)
                                            .emissiveIntensity(10.0f)
                                            .roughness(1.0f)
                                            .metallic(0.0f));
  const MaterialID moonMat =
      materialManager->registerMaterial(MaterialBuilder()
                                            .name("Moon")
                                            .albedoColor(0.8f, 0.8f, 0.9f)
                                            .emissiveIntensity(2.0f)
                                            .roughness(1.0f)
                                            .metallic(0.0f));

  const MeshID sphereMesh = meshManager->createSphere(10.0f, 32);

  Object sunObj;
  ObjectBuilder()
      .name("Sun")
      .position(0.0f, 500.0f, 0.0f)
      .mesh(sphereMesh)
      .material(sunMat)
      .scale(1.5f)
      .layerMask(0x1)
      .build(sunObj);
  sceneObjects.push_back(sunObj);

  Object moonObj;
  ObjectBuilder()
      .name("Moon")
      .position(0.0f, -450.0f, 0.0f)
      .mesh(sphereMesh)
      .material(moonMat)
      .scale(0.7f)
      .layerMask(0x1)
      .build(moonObj);
  sceneObjects.push_back(moonObj);

  const MaterialID sandMat = materialManager->registerMaterial(
      MaterialBuilder()
          .name("Sand")
          .albedoMap("./Models/textures/rockytrail/rocky_trail_02_diff_4k.jpg")
          .normalMap("./Models/textures/rockytrail/rocky_trail_02_ao_4k.jpg")
          .roughnessMap(
              "./Models/textures/rockytrail/rocky_trail_02_arm_4k.jpg")
          .heightMap("./Models/textures/rockytrail/rocky_trail_02_disp_4k.jpg")
          .heightScale(1.0f)
          .textureScale(50.0f));
  const MeshID terrainMesh = meshManager->createProceduralTerrain(
      config.getFloat("Terrain.terrain_size", 300.0f),
      config.getInt("Terrain.terrain_resolution", 100),
      config.getFloat("Terrain.terrain_height", 10.0f),
      config.getFloat("Terrain.terrain_frequency", 2.0f),
      config.getInt("Terrain.terrain_octaves", 2),
      config.getFloat("Terrain.terrain_persistence", 0.6f),
      config.getInt("Terrain.terrain_seed", 42));

  Object terrainObj;
  ObjectBuilder()
      .name("Sand Terrain")
      .position(0.0f, 0.0f, 0.0f)
      .mesh(terrainMesh)
      .material(sandMat)
      .build(terrainObj);
  sceneObjects.push_back(terrainObj);

  const MeshID skyMesh = meshManager->createSphere(100.0f, 64);
  const MaterialID skyMat =
      materialManager->registerMaterial(MaterialBuilder()
                                            .name("Skybox")
                                            .albedoColor(0.93f, 0.08f, 0.08f)
                                            .metallic(0.0f)
                                            .roughness(0.1f)
                                            .transparent(true)
                                            .opacity(0.1f));

  Object skyObj;
  ObjectBuilder()
      .name("Skybox Sphere")
      .position(0.0f, 0.0f, 0.0f)
      .mesh(skyMesh)
      .material(skyMat)
      .scale(3.0f)
      .build(skyObj);
  sceneObjects.push_back(skyObj);

  const MeshID pokeW = meshManager->loadFromOBJ("./Models/PokeWhite.obj");
  const MaterialID pokeWMat =
      materialManager->loadFromMTL("./Models/PokeWhite.mtl");
  const MeshID pokeB = meshManager->loadFromOBJ("./Models/PokeBlack.obj");
  const MaterialID pokeBMat =
      materialManager->loadFromMTL("./Models/PokeBlack.mtl");

  Object pokeWObj;
  ObjectBuilder()
      .name("Pokeball White")
      .position(-10.0f, -130.0f, 0.0f)
      .mesh(pokeW)
      .material(pokeWMat)
      .scale(3.05f)
      .build(pokeWObj);
  sceneObjects.push_back(pokeWObj);

  Object pokeBObj;
  ObjectBuilder()
      .name("Pokeball Black")
      .position(0.0f, 0.0f, 0.0f)
      .mesh(pokeB)
      .material(pokeBMat)
      .scale(3.05f)
      .build(pokeBObj);
  sceneObjects.push_back(pokeBObj);

  Light sunLight;
  LightBuilder()
      .type(LightType::Sun)
      .name("Sun Light")
      .direction(0.0f, -1.0f, 0.0f)
      .color(1.0f, 0.95f, 0.8f)
      .intensity(5.0f)
      .castsShadows(true)
      .build(sunLight);
  sunLightID = lightManager->addLight(sunLight);

  const MaterialID partMat =
      materialManager->registerMaterial(MaterialBuilder()
                                            .name("Particle")
                                            .albedoColor(1.0f, 1.0f, 1.0f)
                                            .roughness(0.0f)
                                            .metallic(0.0f)
                                            .transparent(true));
  plantManager->setFireMaterialID(partMat);

  Light pointlight;
  LightBuilder()
      .type(LightType::Point)
      .name("Point Light")
      .position(0.0f, 0.0f, 0.0f)
      .color(1.0f, 0.95f, 0.8f)
      .attenuation(1.0f, 0.09f, 0.032f)
      .intensity(5.0f)
      .castsShadows(true)
      .build(pointlight);
  lightManager->addLight(pointlight);

  PlantSpawnConfig pConfig;
  pConfig.numCacti = config.getInt("Plants.num_cacti", 400);
  pConfig.numTrees = config.getInt("Plants.num_trees", 100);
  pConfig.minRadius = config.getFloat("Plants.min_spawn_radius", 8.0f);
  pConfig.maxRadius = config.getFloat("Plants.max_spawn_radius", 300.0f);
  pConfig.seed = config.getInt("Plants.spawn_seed", 67);
  pConfig.randomGrowthStages =
      config.getBool("Plants.random_growth_stages", true);
  pConfig.scaleVariance = config.getFloat("Plants.scale_variance", 0.7f);
  pConfig.rotationVariance = config.getFloat("Plants.rotation_variance", 0.8f);

  plantManager->setParticleManager(particleManager);
  plantManager->setTerrainMesh(meshManager->getMesh(terrainMesh));
  plantManager->spawnPlantsOnTerrain(
      sceneObjects, meshManager->getMesh(terrainMesh), pConfig);

  Debug::log(Debug::Category::MAIN, "Created ", sceneObjects.size(),
             " objects, ", lightManager->getLightCount(), " lights, ",
             plantManager->getPlantCount(), " plants");
}

int main() {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  ConfigParser config;
  const bool configLoaded = config.load("config.ini");

#ifdef _DEBUG
  if (!configLoaded)
    Debug::log(Debug::Category::MAIN,
               "Warning: Could not load config.ini, using defaults");
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
#else
  Debug::setEnabled(Debug::Category::MAIN, false);
  Debug::setEnabled(Debug::Category::CAMERA, false);
  Debug::setEnabled(Debug::Category::INPUT, false);
  Debug::setEnabled(Debug::Category::RENDERING, false);
  Debug::setEnabled(Debug::Category::VULKAN, false);
  Debug::setEnabled(Debug::Category::SKYBOX, false);
  Debug::setEnabled(Debug::Category::PLANTMANAGER, false);
  Debug::setEnabled(Debug::Category::WORLD, false);
  Debug::setEnabled(Debug::Category::PARTICLES, false);
  Debug::setEnabled(Debug::Category::MESH, false);
  Debug::setEnabled(Debug::Category::LIGHTS, false);
  Debug::setEnabled(Debug::Category::SCENE, false);
  Debug::setEnabled(Debug::Category::OBJECTS, false);
  Debug::setEnabled(Debug::Category::TEXTURE, false);
  Debug::setEnabled(Debug::Category::MATERIALS, false);
  Debug::setEnabled(Debug::Category::POSTPROCESSING, false);
  Debug::setEnabled(Debug::Category::SHADOWS, false);
#endif

  if (Debug::isEnabled(Debug::Category::MAIN)) printControls();

  try {
    Application app;
    Debug::log(Debug::Category::MAIN, "Initializing Vulkan...");
    app.init();

    Debug::log(Debug::Category::MAIN, "Creating scene...");
    LightID sunLightID = INVALID_LIGHT_ID;
    std::vector<Object> sceneObjects;
    createScene(config, app.getMeshManager(), app.getMaterialManager(),
                app.getPlantManager(), app.getParticleManager(),
                app.getLightManager(), sunLightID, sceneObjects);

    app.setSunLightID(sunLightID);
    app.setScene(sceneObjects);
    app.getLightManager()->debugPrintLightInfo();

    app.getWeatherSystem()->init();
    app.getWeatherSystem()->setSunObject(&sceneObjects[0]);
    app.getWeatherSystem()->setMoonObject(&sceneObjects[1]);

    std::string sunName, moonName;
    sceneObjects[0].getName(sunName);
    sceneObjects[1].getName(moonName);

    Debug::log(Debug::Category::MAIN, "Sun Index 0: ", sunName);
    Debug::log(Debug::Category::MAIN, "Moon Index 1: ", moonName);
    Debug::log(Debug::Category::MAIN, "Total Objects: ", sceneObjects.size());

    WorldConfig wConfig;
    wConfig.dayLengthInSeconds = config.getFloat("World.day_length", 120.0f);
    wConfig.startingHour = config.getInt("World.starting_hour", 12);
    wConfig.startingMinute = config.getInt("World.starting_minute", 0);
    wConfig.startingTemperature =
        config.getFloat("World.starting_temperature", 80.0f);
    wConfig.minTemperature = config.getFloat("World.min_temperature", 70.0f);
    wConfig.maxTemperature = config.getFloat("World.max_temperature", 80.0f);
    wConfig.startingHumidity = config.getFloat("World.starting_humidity", 0.3f);
    wConfig.minHumidity = config.getFloat("World.min_humidity", 0.1f);
    wConfig.maxHumidity = config.getFloat("World.max_humidity", 0.95f);
    wConfig.startingWindSpeed =
        config.getFloat("World.starting_wind_speed", 2.0f);
    wConfig.minWindSpeed = config.getFloat("World.min_wind_speed", 0.5f);
    wConfig.maxWindSpeed = config.getFloat("World.max_wind_speed", 10.0f);
    wConfig.parameterUpdateInterval =
        config.getFloat("World.parameter_update_interval", 30.0f);
    wConfig.dayNightTempVariation =
        config.getFloat("World.day_night_temp_variation", 8.0f);

    app.setWorldConfig(wConfig);

    Debug::log(Debug::Category::MAIN, "Scene created, starting main loop...");
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}