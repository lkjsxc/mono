Of course. Here is the English translation.

# lkjvoxel

## Overview

This project is a simple voxel game developed using C11 and the Vulkan API. It was created for learning purposes, aiming to demonstrate the use of modern C language features and the basics of a low-level graphics API.

The build process is designed to be self-contained within a Docker container, simplifying the development environment setup. It supports multi-platform builds for Windows and Ubuntu.

## Features

  * **Language**: Written in compliance with the C11 standard.
  * **Graphics API**: Uses Vulkan for high-performance rendering.
  * **Cross-Platform Build**: Utilizes Docker to build binaries for both Windows and Ubuntu from a single environment.
  * **Isolated Environment**: All build dependencies are consolidated into a Docker image, keeping the host environment clean.
  * **Modular Source Code**: The codebase is finely divided by functionality, aiming for high readability and maintainability.
  * **Static Memory Allocation**: Avoids dynamic memory allocation during the game loop, primarily using static arrays and fixed-size memory allocators to stabilize performance and suppress memory fragmentation.

## System Requirements

### Runtime Environment

  * Windows 10/11 or Ubuntu 22.04 LTS
  * Graphics driver with support for Vulkan 1.2 or higher

### Build Environment

  * Docker Engine

## Directory Structure

The source code is divided by functionality as shown below.

```
.
├── build/          # Directory for build artifacts
├── cmake/          # CMake-related scripts
│   └── toolchains/   # Toolchain files for cross-compilation
├── src/            # Source code
│   ├── core/         # Core engine features (main loop, time management, etc.)
│   ├── graphics/     # Graphics-related code (Vulkan init, pipelines, rendering)
│   │   ├── vulkan/   # Vulkan API wrappers
│   │   └── models/   # Mesh and texture management
│   ├── input/        # Input handling (keyboard, mouse)
│   ├── platform/     # Platform-dependent code (window creation, event handling)
│   ├── world/        # Game world related (chunk management, block data)
│   └── main.c        # Application entry point
├── third_party/    # External libraries (managed as Git Submodules)
│   ├── cglm/
│   ├── glfw/
│   └── vulkan-headers/
├── .gitignore
├── CMakeLists.txt  # Main CMakeLists.txt
├── Dockerfile
└── README.md
```

### Responsibilities of Each Directory

  * **src/core**: Contains the core logic of the project, such as the game's main loop and application state management.
  * **src/graphics**: Contains all code related to rendering, including Vulkan setup, command buffer generation, graphics pipeline construction, and shader management.
  * **src/input**: Contains code to abstract and process input from the keyboard and mouse.
  * **src/platform**: Isolates platform-dependent code, such as window creation and OS-specific event handling.
  * **src/world**: Contains the data structures for the voxel world (chunks, blocks, etc.) and world generation algorithms.

## Build Instructions

The build process is executed within a Docker container. There is no need to install compilers or libraries on the host machine.

### 1\. Clone the Repository

```bash
git clone --recursive https://github.com/your_username/lkjvoxel.git
cd lkjvoxel
```

Use the `--recursive` option to clone the dependency libraries managed as submodules at the same time.

### 2\. Build the Docker Image

Run the following command in the project's root directory to create the Docker image that will serve as the build environment.

```bash
docker build -t lkjvoxel:latest -f docker/Dockerfile .
```

### 3\. Build the Game

Use the created Docker image to build the binaries for each platform. The build artifacts will be output to the `build/` directory in the project root.

## How to Run

Once the build is complete, executable files for each platform will be generated in the `build/` directory.

### Ubuntu

Execute the file generated in the `build/ubuntu/` directory directly.

```bash
./build/ubuntu/lkjvoxel
```

### Windows

Execute `lkjvoxel.exe` located in the `build/windows/` directory.

## Dependencies

This project depends on the following external libraries. They are included as Git Submodules in the `third_party/` directory and are statically linked during the build.

  * **Vulkan Headers**: Header files for the Vulkan API.
  * **GLFW**: A cross-platform library for window creation, OpenGL/Vulkan context management, and input handling.
  * **cglm**: A highly optimized C math library for OpenGL/Vulkan applications.

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT).