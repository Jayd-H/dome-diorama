# Dome Diorama

Little desert world built with a custom rendering engine in Vulkan.

## Controls

### Camera Controls
- **Enter** - Switch between Orbit and FPS camera modes
- **Right Mouse Button** (Orbit mode) - Rotate camera around scene
- **Mouse Scroll** (Orbit mode) - Zoom in/out
- **W/A/S/D** (FPS mode) - Move forward/left/backward/right
- **Space** (FPS mode) - Move up
- **Shift** (FPS mode) - Move down
- **Mouse Movement** (FPS mode) - Look around
- **Mouse Scroll** (FPS mode) - Adjust movement speed

### Rendering Controls
- **1** - Fill mode (solid rendering)
- **2** - Wireframe mode
- **3** - Point mode
- **4** - Nearest texture filtering
- **5** - Linear texture filtering
- **L** - Toggle between Phong and Gouraud shading

### Weather & Environment Controls
- **T** - Increase temperature (+5°C)
- **G** - Decrease temperature (-5°C)
- **H** - Increase humidity (+10%)
- **N** - Decrease humidity (-10%)
- **U** - Increase wind speed (+1 m/s)
- **J** - Decrease wind speed (-1 m/s)
- **Y** - Cycle through weather states (Clear → Cloudy → Light Rain → Heavy Rain → Light Snow → Heavy Snow → Dust Storm)
- **P** - Pause/Resume time progression

### Weather States
The weather system cycles through these states:
- **Clear** - Sunny skies
- **Cloudy** - Overcast conditions
- **Light Rain** - Light precipitation (rain or snow depending on temperature)
- **Heavy Rain** - Heavy precipitation
- **Light Snow** - Light snowfall (when temperature ≤ 0°C)
- **Heavy Snow** - Heavy snowfall (when temperature ≤ 0°C)
- **Dust Storm** - Sandy wind conditions

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
| .mesh() | MeshID | **Required.** Assigns the geometry ID (created via `MeshManager`). |
| .material() | MaterialID | **Required.** Assigns the look/shader ID (created via `MaterialManager`). |
| .position() | float x, y, z OR glm::vec3 | Sets the object's location in world space. |
| .scale() | float OR glm::vec3 | Sets the size. Passing a single float scales uniformly on all axes. |
| .rotation() | glm::quat | Sets rotation using a quaternion. |
| .rotationEuler() | float p, y, r OR glm::vec3 | Sets rotation using degrees (converted to quaternions internally). |
| .visible() | bool | Toggles rendering visibility (defaults to `true`). |
| .build() | None | Finalizes the configuration and returns the `Object`. |

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
const Object pokeball = ObjectBuilder()
    .name("Pokeball White")
    .position(-10.0f, -130.0f, 0.0f) // Place deep in the scene
   .mesh(pokeballMesh)
    .material(pokeballMat)
    .scale(3.05f)                    // Uniform scale
    .build();

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
    .build();

lightManager->addLight(sun);
```

**API Reference**
| Method | Parameters | Description |
| - | - | - |
| .type() | LightType | `LightType::Sun` (infinite distance) or `LightType::Point` (local bulb). |
| .name() | std::string | Debug name for the light. |
| .color() | float r, g, b OR vec3 | Sets the light color (RGB). |
| .intensity() | float | Brightness multiplier. |
| .position() | float x, y, z OR vec3 | World position (Only affects `Point` lights). |
| .direction() | float x, y, z OR vec3 | Light direction vector (Only affects `Sun` lights). |
| .attenuation() | float c, l, q | Sets falloff: Constant, Linear, Quadratic (Only affects `Point` lights). |
| .castsShadows() | bool | If true, allocates a shadow map for this light. |
| .build() | None | Finalizes the configuration and returns the `Light` object. |

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

## Real-Time Graphics
