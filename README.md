# Dome Diorama

Little desert world simulation built with a custom rendering engine in Vulkan. Lab books for both modules in their respective folders, final lab book at `/FinalLab/FinalLab.md` because I did not want to pollute the root with images.

## Controls

### General Controls
- **ESC** - Exit the application
- **R** - Reset application to initial state (C1 camera, default time/world)
- **K** - Toggle Toon Shader

### Camera Presets
- **F1** - Camera C1: Overview (Globe view)
- **F2** - Camera C2: Navigation (Element view)
- **F3** - Camera C3: Close-up (Cactus view)
- **F4** - Move Camera on and Burn a Random Plant

### Camera Movement
- **Arrow Keys** - Rotate Camera (Left/Right/Up/Down)
- **CTRL + Arrows** - Pan Camera (Left/Right/Forward/Backward)
- **CTRL + PgUp** - Pan Up
- **CTRL + PgDn** - Pan Down
- **Enter** - Switch between Orbit and FPS modes
- **Right Mouse** - Rotate (Orbit mode only)
- **Scroll** - Zoom (Orbit) / Adjust Speed (FPS)

### Effects & Time
- **F4** - Trigger Particle Effect (Random Cactus Fire) & Follow Camera
- **]** - Increase Time Scale (Speed up effects)
- **[** - Decrease Time Scale (Slow down effects)
- **P** - Pause/Resume time progression

### Rendering Controls
- **1** - Fill mode
- **2** - Wireframe mode
- **3** - Point mode
- **4** - Nearest texture filtering
- **5** - Linear texture filtering
- **L** - Toggle Phong/Gouraud shading

### Weather Controls
- **T** - Increase Temperature (+5°C)
- **G** - Decrease Temperature (-5°C)
- **H** - Increase Humidity (+10%)
- **N** - Decrease Humidity (-10%)
- **U** - Increase Wind Speed (+1 m/s)
- **J** - Decrease Wind Speed (-1 m/s)
- **Y** - Cycle Weather State

## C++ Programming & Design

The scene is built in 'CreateScene()'. There are lights, textures, materials, and objects. You can add new things to the scene with a friendly API. 

### Objects

You can create a new entity using the `ObjectBuilder`. 

**Basic Syntax:**
```cpp
const Object myObject = ObjectBuilder()
    .name("Object Name")
    .mesh(myMeshID)
    .material(myMaterialID)
    .position(0.0f, 0.0f, 0.0f)
    .scale(1.0f)
    .build();
```

**API Reference**

| Method | Parameters | Description |
| - | - | - |
| .name() | std::string | Sets a descriptive name for debugging and logging. |
| .mesh() | MeshID | **Required.** Assigns the geometry ID (created via MeshManager). |
| .material() | MaterialID | **Required.** Assigns the look/shader ID (created via MaterialManager). |
| .position() | float x, y, z OR glm::vec3 | Sets the object's location in world space. |
| .scale() | float OR glm::vec3 | Sets the size. Passing a single float scales uniformly on all axes. |
| .rotation() | glm::quat | Sets rotation using a quaternion. |
| .rotationEuler() | float p, y, r OR glm::vec3 | Sets rotation using degrees (converted to quaternions internally). |
| .visible() | bool | Toggles rendering visibility (defaults to true). |
| .layerMask() | uint32_t | Sets rendering layer bits (e.g., 0x1 for specific passes). |
| .build() | Object& | Finalizes the configuration and populates the passed Object. |

#### Post-Creation Manipulation
Once an `Object` is created, you can modify it directly using standard setters. This is useful for game logic or animation updates.

```cpp
 // Example: Moving an object after creation
 myObject.setPosition(10.0f, 5.0f, 0.0f);
 myObject.setScale(2.0f);

// Rotating using Euler angles (Degrees)
 myObject.setRotationEuler(0.0f, 90.0f, 0.0f);
```
#### Complete Example
Here is a practical example of loading a custom mesh and creating an object from it:

```cpp
    // 1. Load the resources
    const MeshID pokeballMesh = meshManager->loadFromOBJ("./Models/PokeWhite.obj");
    const MaterialID pokeballMat = materialManager->loadFromMTL("./Models/PokeWhite.mtl");

    // 2. Build the object
    Object pokeball;
    ObjectBuilder()
        .name("Pokeball White")
        .position(-10.0f, -130.0f, 0.0f) // Place deep in the scene
        .mesh(pokeballMesh)
        .material(pokeballMat)
        .scale(3.05f)                    // Uniform scale
        .build(pokeball);

    // 3. Add to scene
    sceneObjects.push_back(pokeball);
```

### Materials
Materials determine how light interacts with the surfaces of your objects. The engine uses a PBR (Physically Based Rendering) workflow, supporting standard maps for Albedo, Normals, Roughness, Metallic, and Ambient Occlusion.

Materials are created using the `MaterialBuilder` and must be registered with the `MaterialManager` to obtain a `MaterialID`.

**Basic Syntax:**
```cpp
const MaterialID myMaterial = materialManager->registerMaterial(
    MaterialBuilder()
        .name("Gold Material")
        .albedoColor(1.0f, 0.8f, 0.0f) // Yellow-ish
        .metallic(1.0f)                // Fully metallic
        .roughness(0.2f)               // Shiny
);
```

**API Reference**

| Method | Parameters | Description |
| - | - | - |
| .name() | std::string | Debug name for the material. |
| .albedoColor() | float r, g, b OR vec3 | Sets the base color tint (default is White). |
| .albedoMap() | TextureID OR string | Sets the base color texture. |
| .normalMap() | TextureID OR string | Sets the normal map for surface detail. |
| .roughness() | float (0.0 - 1.0) | Sets surface microsurface (0 = smooth/mirror, 1 = matte). |
| .roughnessMap()| TextureID OR string | Sets the roughness texture map. |
| .metallic() | float (0.0 - 1.0) | Sets conductivity (0 = dielectric/plastic, 1 = metal). |
| .metallicMap() | TextureID OR string | Sets the metallic texture map. |
| .emissiveIntensity()| float | Multiplier for emissive light output. |
| .emissiveMap() | TextureID OR string | Sets the texture for self-illumination. |
| .aoMap() | TextureID OR string | Sets the Ambient Occlusion map. |
| .heightMap() | TextureID OR string | Sets the displacement/height map. |
| .heightScale() | float | Multiplier for height map displacement strength. |
| .transparent() | bool | Enables alpha blending (default `false`). |
| .opacity() | float (0.0 - 1.0) | Sets alpha level. `< 1.0` triggers transparency. |
| .textureScale()| float | UV tiling scale. |
| .doubleSided() | bool | Disables backface culling for this material. |

**Loading from .MTL**

The engine supports loading standard Wavefront Material (.mtl) files. This parses the file and automatically loads referenced textures relative to the .mtl file location.

```cpp
// Returns the ID of the first material found in the file
const MaterialID externalMat = materialManager->loadFromMTL("./Models/Castle.mtl");
```

**Complete Example**

Here is a complex material setup using a full set of PBR texture maps:
```cpp
const MaterialID sandMaterialID = materialManager->registerMaterial(
    MaterialBuilder()
        .name("Sand Material")
        // You can pass filepaths directly to the builder
        .albedoMap("./Models/textures/sand_diff_4k.jpg")
        .normalMap("./Models/textures/sand_norm_4k.jpg")
        .roughnessMap("./Models/textures/sand_arm_4k.jpg")
        .heightMap("./Models/textures/sand_disp_4k.jpg")
        // Tweak properties
        .heightScale(0.1f)     // Slight displacement
        .textureScale(50.0f)   // Repeat texture 50 times
        .roughness(1.0f)       // Base roughness multiplier
```

### Lights

The engine supports two primary light types: **Sun** (Directional) and **Point** lights. Lights can also cast dynamic shadows.

Similar to Objects and Materials, lights are constructed using a fluent `LightBuilder` interface. Once built, add them to the `LightManager` to register them in the scene.

**Basic Syntax:**
```cpp
const Light sun = LightBuilder()
    .type(LightType::Sun)
    .name("Main Sun")
    .direction(0.0f, -1.0f, 0.0f) // Pointing straight down
    .intensity(2.0f)
    .castsShadows(true)
    .build(sun);

lightManager->addLight(sun);
```

**API Reference**
| Method | Parameters | Description |
| - | - | - |
| .type() | LightType | LightType::Sun (infinite distance) or LightType::Point (local bulb). |
| .name() | std::string | Debug name for the light. |
| .color() | float r, g, b OR vec3 | Sets the light color (RGB). |
| .intensity() | float | Brightness multiplier. |
| .position() | float x, y, z OR vec3 | World position (Only affects Point lights). |
| .direction() | float x, y, z OR vec3 | Light direction vector (Only affects Sun lights). |
| .attenuation() | float c, l, q | Sets falloff: Constant, Linear, Quadratic (Only affects Point lights). |
| .castsShadows() | bool | If true, allocates a shadow map for this light. |
| .build() | Light& | Finalizes the configuration and populates the passed Light. |

**Light Types Explained**

1. Sun (Directional) Simulates a light source infinitely far away (like the sun). The position is ignored; only direction matters. Rays are parallel.

2. Point Light Simulates a light bulb at a specific location. It emits light in all directions and fades out over distance based on the attenuation settings.

**Complete Example**

Here is how to set up a scene with a main sun and a flickering fire light:

```cpp
// 1. Create the Sun
const Light sunLight = LightBuilder()
    .type(LightType::Sun)
    .name("Sun Light")
    .direction(-0.5f, -1.0f, -0.5f)
    .color(1.0f, 0.95f, 0.8f)      // Warm sunlight
    .intensity(5.0f)
    .castsShadows(true)            // Enable shadows
    .build();

lightManager->addLight(sunLight);

// 2. Create a local fire light
const Light fireLight = LightBuilder()
    .type(LightType::Point)
    .name("Campfire")
    .position(10.0f, 0.5f, 10.0f)
    .color(1.0f, 0.4f, 0.0f)       // Orange fire
    .intensity(3.0f)
    .attenuation(1.0f, 0.09f, 0.032f) // Standard falloff
    .build();

lightManager->addLight(fireLight);
```

### Particle Simulation

The engine features a GPU-accelerated particle system. Particles are managed by emitters, which can be configured to simulate fire, smoke, rain, snow, or dust.

Emitters are created using ParticleEmitterBuilder. Unlike Objects and Lights, the builder **returns a pointer** to a new heap-allocated emitter, which you then pass to ParticleManager.

**Basic Syntax:**
```cpp
    ParticleEmitter* const fireEmitter = ParticleEmitterBuilder()
        .name("Campfire Emitter")
        .position(0.0f, 1.0f, 0.0f)
        .maxParticles(500)
        .baseColor(glm::vec3(1.0f, 0.5f, 0.0f))
        .tipColor(glm::vec3(1.0f, 0.0f, 0.0f))
        .build(); // Returns a new ParticleEmitter*

    // Registering transfers ownership to ParticleManager
    EmitterID id = particleManager->registerEmitter(fireEmitter);
```
**API Reference**

| Method | Parameters | Description |
| - | - | - |
| .name() | std::string | Debug name. |
| .position() | vec3 | World position of the emitter source. |
| .maxParticles() | size_t | Maximum number of active particles. |
| .particleLifetime() | float | How long (seconds) a particle lives. |
| .material() | MaterialID | Material used for the particle quad (usually transparent). |
| .baseColor() | vec3 | Color at the start of the particle's life. |
| .tipColor() | vec3 | Color at the end of the particle's life. |
| .gravity() | vec3 | Constant force applied (e.g., 0, -9.8, 0 for physics). |
| .initialVelocity() | vec3 | Starting velocity vector. |
| .velocityRandomness() | float | Multiplier for randomizing initial velocity. |
| .spawnRadius() | float | Radius around the position where particles spawn. |
| .particleScale() | float | Base size of the particles. |
| .scaleOverLifetime() | float | Multiplier for size change (e.g., 2.0 grows double). |
| .rotationSpeed() | float | Speed of particle rotation. |
| .windInfluence() | float | How strongly global wind affects these particles (0.0 - 1.0). |
| .fadeTimings() | float in, float out | Duration of fade-in and fade-out effects. |
| .billboardMode() | enum | Spherical (faces cam), Cylindrical (upright), or None. |
| .colorMode() | enum | Gradient (lerp base->tip), BaseOnly, or TipOnly. |

**Presets**

The EmitterPresets class provides helpers to quickly configure common effects:
```cpp
    ParticleEmitterBuilder builder;
    // Configures builder for fire, smoke, dust, rain, or snow
    EmitterPresets::createFire(builder); 

    ParticleEmitter* fire = builder
        .position(10.0f, 0.0f, 10.0f) // Override preset position
        .build();

    particleManager->registerEmitter(fire);
```

## Real-Time Graphics
