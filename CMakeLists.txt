cmake_minimum_required(VERSION 3.20)

# Project definition
project(LupineGameEngine 
    VERSION 1.0.0
    DESCRIPTION "Lupine Game Engine - Cross-platform game development framework"
    LANGUAGES CXX C
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Multi-config generators (Visual Studio, Xcode)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# Platform detection
if(WIN32)
    set(LUPINE_PLATFORM "Windows")
    set(LUPINE_PLATFORM_LOWER "windows")
elseif(APPLE)
    set(LUPINE_PLATFORM "Mac-OSX")
    set(LUPINE_PLATFORM_LOWER "macos")
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(LUPINE_PLATFORM "Mac-ARM64")
        set(LUPINE_PLATFORM_LOWER "macos-arm64")
    endif()
elseif(UNIX)
    set(LUPINE_PLATFORM "Linux")
    set(LUPINE_PLATFORM_LOWER "linux")
endif()

message(STATUS "Building for platform: ${LUPINE_PLATFORM}")

# Options
option(LUPINE_ENABLE_EDITOR "Build the Lupine Editor" ON)
option(LUPINE_ENABLE_RUNTIME "Build the Lupine Runtime" ON)
option(LUPINE_STATIC_LINKING "Use static linking for all dependencies" ON)
option(LUPINE_ENABLE_EXPORT "Enable export functionality" ON)

# Compiler-specific settings
if(MSVC)
    # Visual Studio specific settings
    add_compile_options(/W4 /WX- /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    if(LUPINE_STATIC_LINKING)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # GCC/Clang specific settings
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(LUPINE_STATIC_LINKING)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

# Find dependencies
find_package(PkgConfig QUIET)

# Set up third-party library paths
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")
set(PLATFORM_THIRDPARTY_DIR "${THIRDPARTY_DIR}/${LUPINE_PLATFORM}")

# Add platform-specific library and include paths
if(EXISTS "${PLATFORM_THIRDPARTY_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${PLATFORM_THIRDPARTY_DIR}")
    list(APPEND CMAKE_LIBRARY_PATH "${PLATFORM_THIRDPARTY_DIR}/lib")
    list(APPEND CMAKE_INCLUDE_PATH "${PLATFORM_THIRDPARTY_DIR}/include")
    
    # Add to environment for find_package
    set(ENV{PKG_CONFIG_PATH} "${PLATFORM_THIRDPARTY_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    
    message(STATUS "Using platform-specific libraries from: ${PLATFORM_THIRDPARTY_DIR}")
endif()

# Function to recursively find source files
function(lupine_add_sources target_name source_dir)
    file(GLOB_RECURSE SOURCES 
        "${source_dir}/*.cpp"
        "${source_dir}/*.c"
        "${source_dir}/*.cxx"
        "${source_dir}/*.cc"
    )
    
    file(GLOB_RECURSE HEADERS
        "${source_dir}/*.h"
        "${source_dir}/*.hpp"
        "${source_dir}/*.hxx"
    )
    
    target_sources(${target_name} PRIVATE ${SOURCES} ${HEADERS})
    
    # Add include directories
    target_include_directories(${target_name} PRIVATE ${source_dir})
    
    message(STATUS "Added ${CMAKE_CURRENT_FUNCTION_LIST_LINE} sources to ${target_name} from ${source_dir}")
endfunction()

# Function to add platform-specific include directories
function(lupine_add_platform_includes target_name)
    target_include_directories(${target_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/include/lupine"
    )

    if(EXISTS "${PLATFORM_THIRDPARTY_DIR}/include")
        target_include_directories(${target_name} PRIVATE "${PLATFORM_THIRDPARTY_DIR}/include")
    endif()
endfunction()

# Function to find and link libraries
function(lupine_find_and_link_libraries target_name)
    # Core dependencies for all targets
    set(CORE_LIBS "")

    # Platform-specific library linking
    if(WIN32)
        # Windows system libraries
        list(APPEND CORE_LIBS
            kernel32 user32 gdi32 winspool shell32 ole32 oleaut32 uuid comdlg32 advapi32
            winmm ws2_32 opengl32
        )
    elseif(APPLE)
        # macOS frameworks
        find_library(COCOA_FRAMEWORK Cocoa)
        find_library(OPENGL_FRAMEWORK OpenGL)
        find_library(IOKIT_FRAMEWORK IOKit)
        find_library(COREVIDEO_FRAMEWORK CoreVideo)
        find_library(COREAUDIO_FRAMEWORK CoreAudio)
        find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)

        list(APPEND CORE_LIBS
            ${COCOA_FRAMEWORK} ${OPENGL_FRAMEWORK} ${IOKIT_FRAMEWORK}
            ${COREVIDEO_FRAMEWORK} ${COREAUDIO_FRAMEWORK} ${AUDIOTOOLBOX_FRAMEWORK}
        )
    elseif(UNIX)
        # Linux system libraries
        find_package(Threads REQUIRED)
        list(APPEND CORE_LIBS Threads::Threads dl m)

        # X11 and OpenGL
        find_package(X11)
        if(X11_FOUND)
            list(APPEND CORE_LIBS ${X11_LIBRARIES})
        endif()

        find_package(OpenGL)
        if(OPENGL_FOUND)
            list(APPEND CORE_LIBS ${OPENGL_LIBRARIES})
        endif()
    endif()

    target_link_libraries(${target_name} PRIVATE ${CORE_LIBS})
endfunction()

# Create the main engine library
add_library(LupineEngine STATIC)

# Add all engine source files dynamically
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Animation")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Art Tools")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Audio")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Builders")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Components")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Globals")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Input")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Localisation")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Nodes")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Physics")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Production")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Projects")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Rendering")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Scenes")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Scriptable Objects")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Scripting")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Tiles")
lupine_add_sources(LupineEngine "${CMAKE_SOURCE_DIR}/src/Visual Scripting")

# Add include directories and libraries to engine
lupine_add_platform_includes(LupineEngine)
lupine_find_and_link_libraries(LupineEngine)

# Set engine properties
set_target_properties(LupineEngine PROPERTIES
    OUTPUT_NAME "lupine-engine"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

message(STATUS "Configured LupineEngine library")

# Runtime executable
if(LUPINE_ENABLE_RUNTIME)
    add_executable(lupine-runtime)

    # Add runtime-specific sources
    lupine_add_sources(lupine-runtime "${CMAKE_SOURCE_DIR}/src/Runtime")

    # Link with engine library
    target_link_libraries(lupine-runtime PRIVATE LupineEngine)

    # Add includes and platform libraries
    lupine_add_platform_includes(lupine-runtime)
    lupine_find_and_link_libraries(lupine-runtime)

    # Find SDL3 for runtime
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL3 QUIET sdl3)
        if(SDL3_FOUND)
            target_include_directories(lupine-runtime PRIVATE ${SDL3_INCLUDE_DIRS})
            target_link_libraries(lupine-runtime PRIVATE ${SDL3_LIBRARIES})
            target_compile_definitions(lupine-runtime PRIVATE LUPINE_HAS_SDL3)
        endif()
    endif()

    # Try to find SDL3 via find_package if pkg-config failed
    if(NOT SDL3_FOUND)
        find_package(SDL3 QUIET)
        if(SDL3_FOUND)
            target_link_libraries(lupine-runtime PRIVATE SDL3::SDL3)
            target_compile_definitions(lupine-runtime PRIVATE LUPINE_HAS_SDL3)
        endif()
    endif()

    # Set runtime properties
    set_target_properties(lupine-runtime PROPERTIES
        OUTPUT_NAME "lupine-runtime"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Copy icon to output directory
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.ico")
        configure_file("${CMAKE_SOURCE_DIR}/icon.ico" "${CMAKE_BINARY_DIR}/bin/icon.ico" COPYONLY)
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.png")
        configure_file("${CMAKE_SOURCE_DIR}/icon.png" "${CMAKE_BINARY_DIR}/bin/icon.png" COPYONLY)
    endif()

    message(STATUS "Configured lupine-runtime executable")
endif()

# Editor executable
if(LUPINE_ENABLE_EDITOR)
    add_executable(lupine-editor)

    # Add editor-specific sources
    lupine_add_sources(lupine-editor "${CMAKE_SOURCE_DIR}/src/Editor")

    # Link with engine library
    target_link_libraries(lupine-editor PRIVATE LupineEngine)

    # Add includes and platform libraries
    lupine_add_platform_includes(lupine-editor)
    lupine_find_and_link_libraries(lupine-editor)

    # Find Qt6 for editor
    find_package(Qt6 QUIET COMPONENTS Core Widgets Gui)
    if(Qt6_FOUND)
        target_link_libraries(lupine-editor PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)
        target_compile_definitions(lupine-editor PRIVATE LUPINE_HAS_QT6)

        # Enable Qt MOC
        set_target_properties(lupine-editor PROPERTIES
            AUTOMOC ON
            AUTORCC ON
            AUTOUIC ON
        )
    endif()

    # Set editor properties
    set_target_properties(lupine-editor PROPERTIES
        OUTPUT_NAME "lupine-editor"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Copy icon to output directory
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.ico")
        configure_file("${CMAKE_SOURCE_DIR}/icon.ico" "${CMAKE_BINARY_DIR}/bin/icon.ico" COPYONLY)
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.png")
        configure_file("${CMAKE_SOURCE_DIR}/icon.png" "${CMAKE_BINARY_DIR}/bin/icon.png" COPYONLY)
    endif()

    message(STATUS "Configured lupine-editor executable")
endif()

# Installation and packaging
install(TARGETS LupineEngine
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(LUPINE_ENABLE_RUNTIME)
    install(TARGETS lupine-runtime
        RUNTIME DESTINATION bin
    )
endif()

if(LUPINE_ENABLE_EDITOR)
    install(TARGETS lupine-editor
        RUNTIME DESTINATION bin
    )
endif()

# Install headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install icons
if(EXISTS "${CMAKE_SOURCE_DIR}/icon.ico")
    install(FILES "${CMAKE_SOURCE_DIR}/icon.ico" DESTINATION bin)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/icon.png")
    install(FILES "${CMAKE_SOURCE_DIR}/icon.png" DESTINATION bin)
endif()

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "LupineGameEngine")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Lupine Game Engine Team")
set(CPACK_PACKAGE_CONTACT "support@lupine-engine.com")

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Lupine Game Engine")
    set(CPACK_NSIS_PACKAGE_NAME "LupineGameEngine")
    set(CPACK_NSIS_CONTACT "support@lupine-engine.com")
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.ico")
        set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/icon.ico")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/icon.ico")
    endif()
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "Lupine Game Engine")
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.png")
        set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/icon.png")
    endif()
elseif(UNIX)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Lupine Game Engine Team")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
endif()

include(CPack)

# Summary
message(STATUS "")
message(STATUS "=== Lupine Game Engine Build Configuration ===")
message(STATUS "Platform: ${LUPINE_PLATFORM}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Static Linking: ${LUPINE_STATIC_LINKING}")
message(STATUS "Enable Runtime: ${LUPINE_ENABLE_RUNTIME}")
message(STATUS "Enable Editor: ${LUPINE_ENABLE_EDITOR}")
message(STATUS "Enable Export: ${LUPINE_ENABLE_EXPORT}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "Third-party Directory: ${PLATFORM_THIRDPARTY_DIR}")
message(STATUS "===============================================")
message(STATUS "")
