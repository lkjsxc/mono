# `lkjvoxel` Project TODO

This document outlines the planned features, enhancements, and maintenance tasks for the `lkjvoxel` project. It is intended to be a living document that tracks progress and guides future development.

### Priority Legend

-   **Critical:** Essential for core functionality or stability.
-   **High:** Important features for a minimum viable product (MVP).
-   **Medium:** Significant improvements to gameplay or user experience.
-   **Low:** "Nice to have" features, polish, or long-term goals.

## Core Gameplay & Mechanics

This section covers the essential features required to make `lkjvoxel` an interactive game.

-   [ ] **High:** **Block Interaction:** Allow the player to modify the world.
    -   [ ] **High:** Implement ray casting from the camera to determine which block the player is looking at.
    -   [ ] **High:** Add functionality to **destroy** a block (e.g., on left-click).
    -   [ ] **High:** Add functionality to **place** a block (e.g., on right-click).
    -   [ ] **Medium:** After destroying a block, regenerate the chunk mesh to reflect the change.
-   [ ] **Medium:** **Inventory System:** Manage blocks and items the player holds.
    -   [ ] **Medium:** Create a basic hotbar data structure.
    -   [ ] **Medium:** Allow the player to select a block type from the hotbar for placement.
    -   [ ] **Low:** Expand to a full inventory screen.
-   [ ] **Medium:** **Game State Management:** Create a formal state machine in `src/core`.
    -   [ ] **Medium:** Differentiate between states like `MAIN_MENU`, `IN_GAME`, `PAUSED`, and `LOADING`.
    -   [ ] **Medium:** Ensure input and rendering logic behave correctly based on the current state.
-   [ ] **Low:** **World Persistence:** Save and load the game world.
    -   [ ] **Low:** Design a file format for saving chunk data.
    -   [ ] **Low:** Implement functions to serialize chunk data to a file.
    -   [ ] **Low:** Implement functions to load and deserialize chunk data from a file.

## Graphics & Rendering

Improvements to the visual fidelity and rendering performance of the engine.

-   [ ] **High:** **Texture Atlas:** Map multiple block textures onto a single texture sheet.
    -   [ ] **High:** Load a texture atlas image (e.g., PNG).
    -   [ ] **High:** Modify vertex data and shaders to use texture coordinates (`UVs`) to select the correct texture for each block face.
-   [ ] **High:** **Lighting:** Add basic lighting to give the world depth.
    -   [ ] **High:** Implement a simple directional light source to simulate the sun.
    -   [ ] **Medium:** Implement ambient light so that shadows aren't completely black.
    -   [ ] **Medium:** **Ambient Occlusion:** Implement a screen-space ambient occlusion (SSAO) post-processing pass for more realistic lighting in corners and crevices.
-   [ ] **Medium:** **Chunk Mesh Optimization:** Improve how chunk geometry is generated and rendered.
    -   [ ] **Medium:** Implement **Frustum Culling** to avoid rendering chunks that are outside the camera's view.
    -   [ ] **Medium:** Implement **Greedy Meshing** or a similar algorithm to reduce the vertex count by merging adjacent, same-textured block faces into larger quads.
-   [ ] **Medium:** **Advanced Block Types:**
    -   [ ] **Medium:** Support **transparent blocks** like glass and water using a separate rendering pass with blending enabled.
    -   [ ] **Low:** Support **non-standard block shapes** (e.g., slabs, stairs) using custom models or geometry shaders.
-   [ ] **Low:** **Sky & Atmosphere:**
    -   [ ] **Low:** Add a **skybox** or a dynamic **sky dome** with a day/night cycle.
    -   [ ] **Low:** Add atmospheric fog that increases with distance.
-   [ ] **Low:** **Shadows:**
    -   [ ] **Low:** Implement shadow mapping from the main directional light source.

## World Generation & Management

Tasks related to creating and managing the voxel world itself.

-   [ ] **High:** **Procedural World Generation:** Generate terrain algorithmically instead of a flat plane.
    -   [ ] **High:** Integrate a noise library (e.g., OpenSimplex, or implement Perlin noise).
    -   [ ] **High:** Use 2D noise to define terrain height.
    -   [ ] **Medium:** Use 3D noise to generate caves and overhangs.
-   [ ] **Medium:** **Infinite World Streaming:** Dynamically load chunks as the player moves.
    -   [ ] **Medium:** Implement a system to detect when the player moves to a new chunk.
    -   [ ] **Medium:** Create a thread pool for generating new chunk data and meshes in the background to avoid stuttering the main thread.
    -   [ ] **Medium:** Implement a Least Recently Used (LRU) cache to unload distant chunks to manage memory usage.
-   [ ] **Medium:** **Biomes:** Create different environmental regions.
    -   [ ] **Medium:** Use noise maps (e.g., for temperature and humidity) to define biome boundaries.
    -   [ ] **Medium:** Generate different blocks and terrain features based on the biome (e.g., sand for deserts, snow for tundra).
-   [ ] **Low:** **Structure Generation:** Add pre-defined or procedural structures to the world.
    -   [ ] **Low:** Implement logic to place trees.
    -   [ ] **Low:** Design and implement logic for generating villages, dungeons, etc.

## UI & UX (User Interface & Experience)

Adding menus, overlays, and improving player interaction.

-   [ ] **High:** **Debug Overlay:** Display real-time debugging information on screen.
    -   [ ] **High:** Show FPS (Frames Per Second) and frame time.
    -   [ ] **High:** Display player coordinates (`X`, `Y`, `Z`) and current chunk coordinates.
    -   [ ] **Medium:** Consider integrating **Dear ImGui** (`third_party/`) for easier development of complex UI elements.
-   [ ] **High:** **Crosshair:** Add a simple crosshair to the center of the screen to aid with aiming.
-   [ ] **Medium:** **Menus:**
    -   [ ] **Medium:** Create a **Main Menu** (Start Game, Options, Quit).
    -   [ ] **Medium:** Create a **Pause Menu** (Resume, Options, Quit to Main Menu).
-   [ ] **Medium:** **Settings Menu:**
    -   [ ] **Medium:** Allow configuration of mouse sensitivity.
    -   [ ] **Medium:** Allow key re-binding.
    -   [ ] **Low:** Add graphics options (render distance, toggle V-Sync, resolution).

## Code Quality & Refactoring

Improving the health, maintainability, and robustness of the codebase.

-   [ ] **Critical:** **Vulkan Error Handling:** Ensure all Vulkan API calls are properly checked.
    -   [ ] **Critical:** Create a macro or function (e.g., `VK_CHECK(result)`) that validates `VkResult` return values and provides meaningful error messages upon failure.
-   [ ] **High:** **Code Documentation:**
    -   [ ] **High:** Add Doxygen-style comments to all public functions and data structures in header files (`.h`).
    -   [ ] **Medium:** Set up a Doxygen build process to generate HTML documentation.
    -   [ ] **Medium:** Maintain a development log or journal to document key design decisions and implementation details.
-   [ ] **Medium:** **Static Analysis:** Integrate a static analysis tool into the build process.
    -   [ ] **Medium:** Add `clang-tidy` or a similar linter to the CMake build process to catch potential bugs and style issues.
-   [- ] **Medium:** **Unit Testing:**
    -   [ ] **Medium:** Set up a testing framework (e.g., using CMake's CTest).
    -   [ ] **Medium:** Write initial tests for pure logic functions (e.g., math utilities in `cglm`, world coordinate conversions).
-   [ ] **Low:** **Memory Management Audit:**
    -   [ ] **Low:** Review the "static memory allocation" strategy. Profile memory usage during long runs to confirm that fragmentation is suppressed and identify any potential leaks.
    -   [ ] **Low:** Investigate if the fixed-size allocators need resizing or can be made more flexible.

## Build System & CI/CD

Automating and improving the build, testing, and distribution process.

-   [ ] **High:** **Asset Management:**
    -   [ ] **High:** Create an `assets/` directory for textures, shaders, etc.
    -   [ ] **High:** Modify the CMake script to automatically copy assets to the build output directory (`build/windows` and `build/ubuntu`).
-   [ ] **Medium:** **GitHub Actions CI:** Automate the build process.
    -   [ ] **Medium:** Create a workflow file (`.github/workflows/build.yml`).
    -   [ ] **Medium:** Add a job that builds the Docker image.
    -   [ ] **High:** Add a job to test the Docker build on every push.
    -   [ ] **Medium:** Add jobs that execute the Ubuntu and Windows cross-compilation builds on every push to `main` and for every Pull Request.
    -   [ ] **Low:** Extend the CI pipeline to run static analysis and unit tests.
-   [ ] **Low:** **Shader Compilation:**
    -   [ ] **Low:** Pre-compile GLSL shaders to SPIR-V bytecode during the CMake build process using `glslc`, rather than compiling at runtime. This improves startup time and catches shader syntax errors at compile time.