cmake_minimum_required(VERSION 3.21)

# Set CMake policies
cmake_policy(SET CMP0148 NEW)  # Use FindPython instead of deprecated modules

# Set vcpkg toolchain before project declaration
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# Set default vcpkg triplet for static linking
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
    elseif(UNIX AND NOT APPLE)
        set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "")
    elseif(APPLE)
        set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "")
    endif()
endif()

# Project declaration
project(LupineEngine
    VERSION 1.0.0
    DESCRIPTION "Lupine Game Engine - A modern, cross-platform game engine"
    LANGUAGES CXX C
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Platform detection
if(WIN32)
    set(LUPINE_PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(LUPINE_PLATFORM_LINUX TRUE)
elseif(APPLE)
    set(LUPINE_PLATFORM_MACOS TRUE)
endif()

# Build options
option(LUPINE_BUILD_EDITOR "Build the Lupine Editor" ON)
option(LUPINE_BUILD_RUNTIME "Build the Lupine Runtime" ON)
option(LUPINE_BUILD_TESTS "Build unit tests" OFF)
option(LUPINE_ENABLE_PYTHON "Enable Python scripting support" ON)
option(LUPINE_STATIC_RUNTIME "Link runtime libraries statically" ON)

# Set vcpkg features based on options
set(VCPKG_MANIFEST_FEATURES "")
if(LUPINE_BUILD_EDITOR)
    list(APPEND VCPKG_MANIFEST_FEATURES "editor")
endif()
if(LUPINE_ENABLE_PYTHON)
    list(APPEND VCPKG_MANIFEST_FEATURES "python")
endif()

# Compiler-specific settings
if(MSVC)
    # MSVC-specific settings
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    
    # Static runtime linking for Windows
    if(LUPINE_STATIC_RUNTIME OR LUPINE_PLATFORM_WINDOWS)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    
    # Enable parallel compilation
    add_compile_options(/MP)
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang settings
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # Debug settings
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
        add_compile_definitions(DEBUG)
    endif()
    
    # Static linking for Linux
    if(LUPINE_STATIC_RUNTIME AND LUPINE_PLATFORM_LINUX)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

# Global compile definitions
add_compile_definitions(
    LUPINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    LUPINE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    LUPINE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(LUPINE_DEBUG)
endif()

# Platform-specific definitions
if(LUPINE_PLATFORM_WINDOWS)
    add_compile_definitions(LUPINE_PLATFORM_WINDOWS WIN32_LEAN_AND_MEAN NOMINMAX)
elseif(LUPINE_PLATFORM_LINUX)
    add_compile_definitions(LUPINE_PLATFORM_LINUX)
elseif(LUPINE_PLATFORM_MACOS)
    add_compile_definitions(LUPINE_PLATFORM_MACOS)
endif()

# Find packages
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(box2d CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Lua REQUIRED)

# Python support (optional)
if(LUPINE_ENABLE_PYTHON)
    # Use modern FindPython module
    find_package(Python3 COMPONENTS Interpreter Development)
    if(Python3_FOUND)
        # Set pybind11 to use the modern Python finder
        set(PYBIND11_FINDPYTHON ON)
        find_package(pybind11 CONFIG REQUIRED)
        message(STATUS "Python support enabled with Python ${Python3_VERSION}")
    else()
        message(WARNING "Python3 not found, disabling Python support")
        set(LUPINE_ENABLE_PYTHON OFF)
    endif()
endif()
find_package(yaml-cpp CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Audio libraries
find_package(SndFile CONFIG REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(FLAC CONFIG REQUIRED)
find_package(opus CONFIG REQUIRED)

# Image libraries
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Freetype REQUIRED)

# Compression libraries
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

# Qt for editor (optional)
if(LUPINE_BUILD_EDITOR)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
endif()

# Add subdirectories
add_subdirectory(lupine-core)

if(LUPINE_BUILD_RUNTIME)
    add_subdirectory(lupine-runtime)
endif()

if(LUPINE_BUILD_EDITOR)
    add_subdirectory(lupine-engine)
endif()

# Install configuration
include(GNUInstallDirs)

# Export targets
install(EXPORT LupineTargets
    FILE LupineTargets.cmake
    NAMESPACE Lupine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Lupine
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LupineConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/LupineConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Lupine
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/LupineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LupineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LupineConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Lupine
)

# Print configuration summary
message(STATUS "")
message(STATUS "Lupine Engine Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Build Editor: ${LUPINE_BUILD_EDITOR}")
message(STATUS "  Build Runtime: ${LUPINE_BUILD_RUNTIME}")
message(STATUS "  Build Tests: ${LUPINE_BUILD_TESTS}")
message(STATUS "  Enable Python: ${LUPINE_ENABLE_PYTHON}")
message(STATUS "  Static Runtime: ${LUPINE_STATIC_RUNTIME}")
message(STATUS "")
