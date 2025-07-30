# Windows Cross-Compilation Guide for LKJVoxel

This guide explains how to build Windows executables for LKJVoxel using cross-compilation from Linux.

## 🎯 Overview

LKJVoxel supports Windows cross-compilation through two methods:
1. **Docker-based compilation** (Recommended)
2. **Native MinGW compilation** (Direct)

## 🐳 Method 1: Docker-based Cross-Compilation (Recommended)

### Prerequisites
- Docker installed on your system
- At least 2GB free disk space for Docker images

### Quick Start
```bash
# Make the script executable
chmod +x build_windows.sh

# Build Windows executable
./build_windows.sh Release
```

### What happens:
1. **Docker Image Creation**: Creates a Ubuntu-based image with MinGW cross-compilation tools
2. **Dependency Building**: Compiles GLFW and Vulkan headers for Windows
3. **LKJVoxel Compilation**: Cross-compiles the engine to Windows executable
4. **Output**: Produces `build-windows/LKJVoxel.exe`

### Docker Image Contents:
- Ubuntu 24.04 base
- MinGW-w64 cross-compilation toolchain
- CMake and Ninja build system
- Pre-built GLFW 3.3.8 for Windows
- Vulkan SDK headers
- Custom CMake toolchain file

### Build Commands:
```bash
# Debug build
./build_windows.sh Debug

# Release build (default)
./build_windows.sh Release
./build_windows.sh
```

## 🔧 Method 2: Native MinGW Cross-Compilation

### Prerequisites
Install MinGW-w64 on Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y \
    mingw-w64 \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64 \
    cmake \
    ninja-build
```

### Build Process
```bash
# Make the script executable
chmod +x build_windows_native.sh

# Build Windows executable
./build_windows_native.sh Release
```

### Manual Build Steps:
```bash
# Create build directory
mkdir build-windows-manual
cd build-windows-manual

# Configure with MinGW toolchain
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTS=OFF \
      ..

# Build
make -j$(nproc)
```

## 📁 Files and Scripts

### Build Scripts:
- `build_windows.sh` - Docker-based cross-compilation
- `build_windows_native.sh` - Native MinGW cross-compilation

### Docker Files:
- `Dockerfile.windows` - Multi-stage Docker build
- `Dockerfile.windows-simple` - Simplified Docker build
- `docker-compose.yml` - Docker Compose configuration

### CMake Files:
- `cmake/mingw-toolchain.cmake` - MinGW toolchain configuration
- `CMakeLists.txt` - Updated with Windows cross-compilation support

## 🔧 CMake Toolchain Configuration

The MinGW toolchain file (`cmake/mingw-toolchain.cmake`) configures:
- **Target System**: Windows x86_64
- **Compilers**: MinGW-w64 GCC/G++
- **Linking**: Static linking to avoid DLL dependencies
- **Libraries**: Windows-specific library paths

Key settings:
```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
```

## 📦 Dependencies

### Automatically Handled:
- **GLFW 3.3.8**: Cross-compiled and statically linked
- **Vulkan Headers**: Cross-compiled for Windows
- **Standard Libraries**: Statically linked

### Runtime Requirements (Windows):
- **Vulkan Runtime**: Must be installed on target Windows system
- **Windows 10/11**: Recommended target OS

## 🎯 Output

### Successful Build Produces:
```
build-windows/
├── LKJVoxel.exe          # Main executable (~100KB)
├── CMakeCache.txt        # Build configuration
└── ...                   # Build artifacts
```

### Executable Characteristics:
- **Architecture**: x86_64 (64-bit)
- **Dependencies**: Self-contained (static linking)
- **Size**: ~100KB (depending on build type)
- **Runtime**: Requires Vulkan runtime on Windows

## 🐛 Troubleshooting

### Common Issues:

#### 1. Docker Build Fails
```bash
# Clean Docker cache
docker system prune -f
docker rmi lkjvoxel-windows-builder

# Rebuild from scratch
./build_windows.sh
```

#### 2. MinGW Not Found
```bash
# Install MinGW-w64
sudo apt-get install mingw-w64 gcc-mingw-w64-x86-64
```

#### 3. GLFW/Vulkan Not Found
The build scripts automatically handle these dependencies. If issues persist:
- Use Docker method (recommended)
- Check `/usr/x86_64-w64-mingw32/` directory

#### 4. Linking Errors
Ensure static linking is enabled in the toolchain file:
```cmake
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
```

### Verbose Debugging:
```bash
# Enable verbose CMake output
cmake --build . --verbose

# Check generated Makefile
cat Makefile
```

## ✅ Testing

### On Windows:
1. Copy `LKJVoxel.exe` to Windows machine
2. Install Vulkan runtime if not present
3. Run from command prompt:
   ```cmd
   LKJVoxel.exe
   ```

### Expected Output:
- Application should start and attempt to initialize
- In headless environment: Clean error about missing display
- With display: Window creation and Vulkan initialization

## 🚀 Advanced Usage

### Multi-Platform Build:
```bash
# Build both Linux and Windows
./build.sh Release          # Linux
./build_windows.sh Release  # Windows

# Compare outputs
ls -la build/LKJVoxel build-windows/LKJVoxel.exe
```

### Custom Build Configuration:
```bash
# Custom CMake flags
docker run --rm -v $(pwd):/src -w /src lkjvoxel-windows-builder \
  cmake -DCMAKE_TOOLCHAIN_FILE=/build/mingw-toolchain.cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCUSTOM_FLAG=ON \
        ..
```

## 📊 Performance Notes

### Build Times:
- **Docker Image Creation**: ~5-10 minutes (first time)
- **Subsequent Builds**: ~30-60 seconds
- **Native MinGW**: ~30 seconds

### Resource Usage:
- **Disk Space**: ~500MB for Docker image
- **RAM**: ~1GB during build
- **CPU**: Uses all available cores (`-j$(nproc)`)

## 🔐 Security Considerations

### Static Linking:
- Pros: Self-contained executable, no DLL hell
- Cons: Larger file size, security updates require rebuild

### Vulkan Runtime:
- Must be installed separately on target Windows system
- Available from GPU vendors (NVIDIA, AMD, Intel)

This setup provides a robust, automated Windows cross-compilation workflow for the LKJVoxel engine while maintaining compatibility with the existing Linux development environment.
