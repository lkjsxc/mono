# `lkjvoxel` Project TODO

This document outlines the planned features, enhancements, and maintenance tasks for the `lkjvoxel` project. It is intended to be a living document that tracks progress and guides future development.

### Priority Legend

-   🔴 **Critical:** Essential for core functionality or stability.
-   🟠 **High:** Important features for a minimum viable product (MVP).
-   🟡 **Medium:** Significant improvements to gameplay or user experience.
--   🔵 **Low:** "Nice to have" features, polish, or long-term goals.

## ⚙️ Core Gameplay & Mechanics

This section covers the essential features required to make `lkjvoxel` an interactive game.

-   [ ] 🔴 **Player Physics & Collision:** Implement basic physics for the player.
    -   [ ] 🔴 Add gravity to pull the player downwards.
    -   [ ] 🔴 Implement AABB (Axis-Aligned Bounding Box) collision detection between the player and world blocks to prevent moving through terrain.
    -   [ ] 🟡 Implement a more sophisticated player controller (e.g., walking vs. flying mode, jumping, sprinting).
-   [ ] 🟠 **Block Interaction:** Allow the player to modify the world.
    -   [ ] 🟠 Implement ray casting from the camera to determine which block the player is looking at.
    -   [ ] 🟠 Add functionality to **destroy** a block (e.g., on left-click).
    -   [ ] 🟠 Add functionality to **place** a block (e.g., on right-click).
    -   [ ] 🟡 After destroying a block, regenerate the chunk mesh to reflect the change.
-   [ ] 🟡 **Inventory System:** Manage blocks and items the player holds.
    -   [ ] 🟡 Create a basic hotbar data structure.
    -   [ ] 🟡 Allow the player to select a block type from the hotbar for placement.
    -   [ ] 🔵 Expand to a full inventory screen.
-   [ ] 🟡 **Game State Management:** Create a formal state machine in `src/core`.
    -   [ ] 🟡 Differentiate between states like `MAIN_MENU`, `IN_GAME`, `PAUSED`, and `LOADING`.
    -   [ ] 🟡 Ensure input and rendering logic behave correctly based on the current state.
-   [ ] 🔵 **World Persistence:** Save and load the game world.
    -   [ ] 🔵 Design a file format for saving chunk data.
    -   [ ] 🔵 Implement functions to serialize chunk data to a file.
    -   [ ] 🔵 Implement functions to load and deserialize chunk data from a file.

## 🎨 Graphics & Rendering

Improvements to the visual fidelity and rendering performance of the engine.

-   [ ] 🟠 **Texture Atlas:** Map multiple block textures onto a single texture sheet.
    -   [ ] 🟠 Load a texture atlas image (e.g., PNG).
    -   [ ] 🟠 Modify vertex data and shaders to use texture coordinates (`UVs`) to select the correct texture for each block face.
-   [ ] 🟠 **Lighting:** Add basic lighting to give the world depth.
    -   [ ] 🟠 Implement a simple directional light source to simulate the sun.
    -   [ ] 🟡 Implement ambient light so that shadows aren't completely black.
    -   [ ] 🟡 **Ambient Occlusion:** Implement a screen-space ambient occlusion (SSAO) post-processing pass for more realistic lighting in corners and crevices.
-   [ ] 🟡 **Chunk Mesh Optimization:** Improve how chunk geometry is generated and rendered.
    -   [ ] 🟡 Implement **Frustum Culling** to avoid rendering chunks that are outside the camera's view.
    -   [ ] 🟡 Implement **Greedy Meshing** or a similar algorithm to reduce the vertex count by merging adjacent, same-textured block faces into larger quads.
-   [ ] 🟡 **Advanced Block Types:**
    -   [ ] 🟡 Support **transparent blocks** like glass and water using a separate rendering pass with blending enabled.
    -   [ ] 🔵 Support **non-standard block shapes** (e.g., slabs, stairs) using custom models or geometry shaders.
-   [ ] 🔵 **Sky & Atmosphere:**
    -   [ ] 🔵 Add a **skybox** or a dynamic **sky dome** with a day/night cycle.
    -   [ ] 🔵 Add atmospheric fog that increases with distance.
-   [ ] 🔵 **Shadows:**
    -   [ ] 🔵 Implement shadow mapping from the main directional light source.

## 🌍 World Generation & Management

Tasks related to creating and managing the voxel world itself.

-   [ ] 🟠 **Procedural World Generation:** Generate terrain algorithmically instead of a flat plane.
    -   [ ] 🟠 Integrate a noise library (e.g., OpenSimplex, or implement Perlin noise).
    -   [ ] 🟠 Use 2D noise to define terrain height.
    -   [ ] 🟡 Use 3D noise to generate caves and overhangs.
-   [ ] 🟡 **Infinite World Streaming:** Dynamically load chunks as the player moves.
    -   [ ] 🟡 Implement a system to detect when the player moves to a new chunk.
    -   [ ] 🟡 Create a thread pool for generating new chunk data and meshes in the background to avoid stuttering the main thread.
    -   [ ] 🟡 Implement a Least Recently Used (LRU) cache to unload distant chunks to manage memory usage.
-   [ ] 🟡 **Biomes:** Create different environmental regions.
    -   [ ] 🟡 Use noise maps (e.g., for temperature and humidity) to define biome boundaries.
    -   [ ] 🟡 Generate different blocks and terrain features based on the biome (e.g., sand for deserts, snow for tundra).
-   [ ] 🔵 **Structure Generation:** Add pre-defined or procedural structures to the world.
    -   [ ] 🔵 Implement logic to place trees.
    -   [ ] 🔵 Design and implement logic for generating villages, dungeons, etc.

## 🖥️ UI & UX (User Interface & Experience)

Adding menus, overlays, and improving player interaction.

-   [ ] 🟠 **Debug Overlay:** Display real-time debugging information on screen.
    -   [ ] 🟠 Show FPS (Frames Per Second) and frame time.
    -   [ ] 🟠 Display player coordinates (`X`, `Y`, `Z`) and current chunk coordinates.
    -   [ ] 🟡 Consider integrating **Dear ImGui** (`third_party/`) for easier development of complex UI elements.
-   [ ] 🟠 **Crosshair:** Add a simple crosshair to the center of the screen to aid with aiming.
-   [ ] 🟡 **Menus:**
    -   [ ] 🟡 Create a **Main Menu** (Start Game, Options, Quit).
    -   [ ] 🟡 Create a **Pause Menu** (Resume, Options, Quit to Main Menu).
-   [ ] 🟡 **Settings Menu:**
    -   [ ] 🟡 Allow configuration of mouse sensitivity.
    -   [ ] 🟡 Allow key re-binding.
    -   [ ] 🔵 Add graphics options (render distance, toggle V-Sync, resolution).

## 🛠️ Code Quality & Refactoring

Improving the health, maintainability, and robustness of the codebase.

-   [ ] 🔴 **Vulkan Error Handling:** Ensure all Vulkan API calls are properly checked.
    -   [ ] 🔴 Create a macro or function (e.g., `VK_CHECK(result)`) that validates `VkResult` return values and provides meaningful error messages upon failure.
-   [ ] 🟠 **Code Documentation:**
    -   [ ] 🟠 Add Doxygen-style comments to all public functions and data structures in header files (`.h`).
    -   [ ] 🟡 Set up a Doxygen build process to generate HTML documentation.
-   [ ] 🟡 **Static Analysis:** Integrate a static analysis tool into the build process.
    -   [ ] 🟡 Add `clang-tidy` or a similar linter to the CMake build process to catch potential bugs and style issues.
-   [- ] 🟡 **Unit Testing:**
    -   [ ] 🟡 Set up a testing framework (e.g., using CMake's CTest).
    -   [ ] 🟡 Write initial tests for pure logic functions (e.g., math utilities in `cglm`, world coordinate conversions).
-   [ ] 🔵 **Memory Management Audit:**
    -   [ ] 🔵 Review the "static memory allocation" strategy. Profile memory usage during long runs to confirm that fragmentation is suppressed and identify any potential leaks.
    -   [ ] 🔵 Investigate if the fixed-size allocators need resizing or can be made more flexible.

## 🏗️ Build System & CI/CD

Automating and improving the build, testing, and distribution process.

-   [ ] 🟠 **Asset Management:**
    -   [ ] 🟠 Create an `assets/` directory for textures, shaders, etc.
    -   [ ] 🟠 Modify the CMake script to automatically copy assets to the build output directory (`build/windows` and `build/ubuntu`).
-   [ ] 🟡 **GitHub Actions CI:** Automate the build process.
    -   [ ] 🟡 Create a workflow file (`.github/workflows/build.yml`).
    -   [ ] 🟡 Add a job that builds the Docker image.
    -   [ ] 🟡 Add jobs that execute the Ubuntu and Windows cross-compilation builds on every push to `main` and for every Pull Request.
    -   [ ] 🔵 Extend the CI pipeline to run static analysis and unit tests.
-   [ ] 🔵 **Shader Compilation:**
    -   [ ] 🔵 Pre-compile GLSL shaders to SPIR-V bytecode during the CMake build process using `glslc`, rather than compiling at runtime. This improves startup time and catches shader syntax errors at compile time.