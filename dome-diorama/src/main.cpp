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
#include "Util/Debug.h"

#ifdef NDEBUG
const bool DEBUG_MAIN = false;
const bool DEBUG_CAMERA = false;
const bool DEBUG_INPUT = false;
const bool DEBUG_RENDERING = false;
const bool DEBUG_VULKAN = false;
const bool DEBUG_SKYBOX = false;
const bool DEBUG_PLANTMANAGER = false;
const bool DEBUG_WORLD = false;
const bool DEBUG_PARTICLES = false;
const bool DEBUG_MESH = false;
const bool DEBUG_LIGHTS = false;
const bool DEBUG_SCENE = false;
const bool DEBUG_OBJECTS = false;
const bool DEBUG_TEXTURE = false;
const bool DEBUG_MATERIALS = false;
const bool DEBUG_POSTPROCESSING = false;
const bool DEBUG_SHADOWS = false;
#else
const bool DEBUG_MAIN = true;
const bool DEBUG_CAMERA = false;
const bool DEBUG_INPUT = false;
const bool DEBUG_RENDERING = true;
const bool DEBUG_VULKAN = true;
const bool DEBUG_SKYBOX = true;
const bool DEBUG_PLANTMANAGER = true;
const bool DEBUG_WORLD = true;
const bool DEBUG_PARTICLES = true;
const bool DEBUG_MESH = false;
const bool DEBUG_LIGHTS = true;
const bool DEBUG_SCENE = true;
const bool DEBUG_OBJECTS = false;
const bool DEBUG_TEXTURE = false;
const bool DEBUG_MATERIALS = false;
const bool DEBUG_POSTPROCESSING = true;
const bool DEBUG_SHADOWS = true;
#endif

std::vector<Object> createScene(MeshManager* meshManager,
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
      300.0f, 100, 10.0f, 2.0f, 2, 0.6f, 42);

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
  plantConfig.numCacti = 400;
  plantConfig.numTrees = 100;
  plantConfig.minRadius = 8.0f;
  plantConfig.maxRadius = 300.0f;
  plantConfig.seed = 67;
  plantConfig.randomGrowthStages = true;
  plantConfig.scaleVariance = 0.7f;
  plantConfig.rotationVariance = 0.8f;

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
                                       .waveFrequency(2.0f)
                                       .waveAmplitude(0.5f)
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
                                       .swirlingRadius(3.0f)
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

  Debug::setEnabled(Debug::Category::MAIN, DEBUG_MAIN);
  Debug::setEnabled(Debug::Category::CAMERA, DEBUG_CAMERA);
  Debug::setEnabled(Debug::Category::INPUT, DEBUG_INPUT);
  Debug::setEnabled(Debug::Category::RENDERING, DEBUG_RENDERING);
  Debug::setEnabled(Debug::Category::VULKAN, DEBUG_VULKAN);
  Debug::setEnabled(Debug::Category::SKYBOX, DEBUG_SKYBOX);
  Debug::setEnabled(Debug::Category::PLANTMANAGER, DEBUG_PLANTMANAGER);
  Debug::setEnabled(Debug::Category::WORLD, DEBUG_WORLD);
  Debug::setEnabled(Debug::Category::PARTICLES, DEBUG_PARTICLES);
  Debug::setEnabled(Debug::Category::MESH, DEBUG_MESH);
  Debug::setEnabled(Debug::Category::LIGHTS, DEBUG_LIGHTS);
  Debug::setEnabled(Debug::Category::SCENE, DEBUG_SCENE);
  Debug::setEnabled(Debug::Category::OBJECTS, DEBUG_OBJECTS);
  Debug::setEnabled(Debug::Category::TEXTURE, DEBUG_TEXTURE);
  Debug::setEnabled(Debug::Category::MATERIALS, DEBUG_MATERIALS);
  Debug::setEnabled(Debug::Category::POSTPROCESSING, DEBUG_POSTPROCESSING);
  Debug::setEnabled(Debug::Category::SHADOWS, DEBUG_SHADOWS);

  try {
    Application app;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}