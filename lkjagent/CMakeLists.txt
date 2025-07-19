cmake_minimum_required(VERSION 3.10)
project(lkjagent VERSION 1.0.0 DESCRIPTION "LKJAgent - Autonomous AI Agent System")

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags for production-grade quality
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wno-nonnull-compare")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

# Include directories
include_directories(src/include)

# Source files
set(CORE_SOURCES
    src/utils/data.c
    src/utils/tag_parser.c
    src/utils/file_io.c
    src/utils/json_parser.c
    src/utils/json_builder.c
    src/config/config_loader.c
    src/persistence/persist_memory.c
)

# Create static library
add_library(lkjagent_core STATIC ${CORE_SOURCES})

# Test executable
add_executable(test_core_infrastructure
    tests/test_core_infrastructure.c
)

target_link_libraries(test_core_infrastructure lkjagent_core m)

# Enable testing
enable_testing()
add_test(NAME CoreInfrastructureTest COMMAND test_core_infrastructure)

# Installation
install(TARGETS lkjagent_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES src/lkjagent.h DESTINATION include)
install(DIRECTORY src/include/ DESTINATION include/lkjagent)
