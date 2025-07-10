cmake_minimum_required(VERSION 3.20)

project(LupineEngine VERSION 1.0.0 LANGUAGES CXX)

# =============================================================================
# STATIC LINKING CONFIGURATION
# =============================================================================
# This CMake configuration ensures ALL libraries are statically linked to
# prevent DLL distribution issues. The following libraries are now statically
# linked and should not require DLLs on target machines:
#
# Core Libraries:
# - SDL2, SDL2_image, SDL2_ttf, SDL2_mixer (graphics, audio, input)
# - Lua, Python3 (scripting)
# - Assimp (3D model loading)
# - Bullet3, Box2D (physics)
# - OpenGL/GLAD (graphics)
#
# Audio Libraries:
# - libvorbis, libogg, libflac, mpg123, opus, libstk, libsndfile
# - mp3lame (MP3 encoding)
# - wavpack (audio compression)
#
# Compression/Archive Libraries:
# - zlib, bzip2, lzma, lz4, zstd, brotli
# - minizip (ZIP archive support)
#
# Image Libraries:
# - libpng, libjpeg-turbo, freetype (fonts)
#
# Qt6 Libraries (Editor only):
# - Qt6Core, Qt6Widgets, Qt6Gui, Qt6OpenGL, Qt6OpenGLWidgets
# - All Qt dependencies: harfbuzz, ICU, PCRE2, double-conversion
#
# Utility Libraries:
# - yaml-cpp, nlohmann-json, pugixml (data formats)
# - spdlog (logging)
# - polyclipping, poly2tri (geometry)
# - sqlite3 (database)
# - openssl (SSL/crypto)
#
# All libraries prioritize static (.lib) versions over dynamic linking.
# =============================================================================

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force static runtime linking for Windows to match static libraries
if(WIN32 AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # Add required compiler flags for Qt and static linking
    add_compile_options(/Zc:__cplusplus)  # Required for Qt to detect C++17 properly
    add_compile_options(/permissive-)     # Required for Qt modern C++ compliance

    # Alternative approach for older CMake versions
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Set third-party directory
set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# Find required packages (PkgConfig not needed on Windows)
if(NOT WIN32)
    find_package(PkgConfig REQUIRED)
endif()

# Set CMAKE_PREFIX_PATH to include all third-party directories (prefer static versions)
# Include both Windows and Linux library paths for cross-platform export support
list(APPEND CMAKE_PREFIX_PATH
    # Linux static libraries (for Linux builds and Windows->Linux export)
    "${THIRDPARTY_DIR}/linux_x64-static/sdl2"
    "${THIRDPARTY_DIR}/linux_x64-static/sdl2-image"
    "${THIRDPARTY_DIR}/linux_x64-static/sdl2-ttf"
    "${THIRDPARTY_DIR}/linux_x64-static/sdl2-mixer"
    "${THIRDPARTY_DIR}/linux_x64-static/assimp"
    "${THIRDPARTY_DIR}/linux_x64-static/box2d"
    "${THIRDPARTY_DIR}/linux_x64-static/bullet3"
    "${THIRDPARTY_DIR}/linux_x64-static/lua"
    "${THIRDPARTY_DIR}/linux_x64-static/yaml-cpp"
    "${THIRDPARTY_DIR}/linux_x64-static/pugixml"
    "${THIRDPARTY_DIR}/linux_x64-static/python3"
    "${THIRDPARTY_DIR}/linux_x64-static/glad"
    "${THIRDPARTY_DIR}/linux_x64-static/spdlog"
    "${THIRDPARTY_DIR}/linux_x64-static/nlohmann-json"
    # Windows static libraries (existing)
    "${THIRDPARTY_DIR}/minizip_x64-windows-static"
    "${THIRDPARTY_DIR}/minizip_x64-windows"
    "${THIRDPARTY_DIR}/sdl2_x64-windows-static"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows-static"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows-static"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows-static"
    "${THIRDPARTY_DIR}/libstk_x64-windows-static"
    "${THIRDPARTY_DIR}/libvorbis_x64-windows-static"
    "${THIRDPARTY_DIR}/libogg_x64-windows-static"
    "${THIRDPARTY_DIR}/libflac_x64-windows-static"
    "${THIRDPARTY_DIR}/mp3lame_x64-windows-static"
    "${THIRDPARTY_DIR}/mpg123_x64-windows-static"
    "${THIRDPARTY_DIR}/opus_x64-windows-static"
    "${THIRDPARTY_DIR}/bzip2_x64-windows-static"
    "${THIRDPARTY_DIR}/libjpeg-turbo_x64-windows-static"
    "${THIRDPARTY_DIR}/lz4_x64-windows-static"
    "${THIRDPARTY_DIR}/wavpack_x64-windows-static"
    "${THIRDPARTY_DIR}/sqlite3_x64-windows-static"
    "${THIRDPARTY_DIR}/openssl_x64-windows-static"
    "${THIRDPARTY_DIR}/glm_x64-windows"
    "${THIRDPARTY_DIR}/sdl2_x64-windows"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows"
    "${THIRDPARTY_DIR}/libstk_x64-windows"
    "${THIRDPARTY_DIR}/libvorbis_x64-windows"
    "${THIRDPARTY_DIR}/libogg_x64-windows"
    "${THIRDPARTY_DIR}/libflac_x64-windows"
    "${THIRDPARTY_DIR}/mp3lame_x64-windows"
    "${THIRDPARTY_DIR}/mpg123_x64-windows"
    "${THIRDPARTY_DIR}/opus_x64-windows"
    "${THIRDPARTY_DIR}/assimp_x64-windows-static"
    "${THIRDPARTY_DIR}/assimp_x64-windows"
    "${THIRDPARTY_DIR}/yaml-cpp_x64-windows"
    "${THIRDPARTY_DIR}/boost-uuid_x64-windows"
    "${THIRDPARTY_DIR}/nlohmann-json_x64-windows"
    "${THIRDPARTY_DIR}/lua_x64-windows-static"
    "${THIRDPARTY_DIR}/lua_x64-windows"
    "${THIRDPARTY_DIR}/python3_x64-windows-static"
    "${THIRDPARTY_DIR}/python3_x64-windows"
    "${THIRDPARTY_DIR}/glad_x64-windows"
    "${THIRDPARTY_DIR}/stb_x64-windows-static"
    "${THIRDPARTY_DIR}/stb_x64-windows"
    "${THIRDPARTY_DIR}/spdlog_x64-windows"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows-static"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows"
    "${THIRDPARTY_DIR}/box2D"
    "${THIRDPARTY_DIR}/bullet3_x64-windows"
)

# Try to find packages, but make them optional for now to get basic build working
find_package(SDL2 QUIET)
find_package(SDL2_image QUIET)
find_package(SDL2_ttf QUIET)
find_package(glm QUIET)
find_package(assimp QUIET)
find_package(yaml-cpp QUIET)
find_package(nlohmann_json QUIET)
find_package(spdlog QUIET)

# Find Qt6 for the editor (use static version)
set(Qt6_DIR "${THIRDPARTY_DIR}/qtbase_x64-windows-static/share/Qt6")
set(Qt6Core_DIR "${THIRDPARTY_DIR}/qtbase_x64-windows-static/share/Qt6Core")
set(Qt6Widgets_DIR "${THIRDPARTY_DIR}/qtbase_x64-windows-static/share/Qt6Widgets")
set(Qt6Gui_DIR "${THIRDPARTY_DIR}/qtbase_x64-windows-static/share/Qt6Gui")
set(Qt6OpenGL_DIR "${THIRDPARTY_DIR}/qtbase_x64-windows-static/share/Qt6OpenGL")
set(Qt6OpenGLWidgets_DIR "${THIRDPARTY_DIR}/qtbase_x64-windows-static/share/Qt6OpenGLWidgets")

# Set Qt to static linking mode and host path for cross-compilation
# Use dynamic Qt for tools (moc, uic, rcc) but static Qt for libraries
set(QT_USE_STATIC_LIBS ON)
set(QT_HOST_PATH "${THIRDPARTY_DIR}/qtbase_x64-windows" CACHE PATH "Qt host path for tools")
set(QT_HOST_PATH_CMAKE_DIR "${THIRDPARTY_DIR}/qtbase_x64-windows/share" CACHE PATH "Qt host cmake dir for tools")

# Manually set Qt tool paths
set(QT_MOC_EXECUTABLE "${THIRDPARTY_DIR}/qtbase_x64-windows/tools/Qt6/bin/moc.exe" CACHE FILEPATH "Path to moc executable")
set(QT_UIC_EXECUTABLE "${THIRDPARTY_DIR}/qtbase_x64-windows/tools/Qt6/bin/uic.exe" CACHE FILEPATH "Path to uic executable")
set(QT_RCC_EXECUTABLE "${THIRDPARTY_DIR}/qtbase_x64-windows/tools/Qt6/bin/rcc.exe" CACHE FILEPATH "Path to rcc executable")

# Force static linking for Qt
add_definitions(-DQT_STATICPLUGIN)
add_definitions(-DQT_STATIC)

# Force static linking for SDL2
add_definitions(-DSDL_STATIC_LIB)

# Cross-platform library embedding options
option(LUPINE_EMBED_LINUX_LIBRARIES "Embed Linux static libraries for cross-platform export" ON)
option(LUPINE_EMBED_WINDOWS_LIBRARIES "Embed Windows static libraries for cross-platform export" ON)

# Configure Linux library embedding
if(LUPINE_EMBED_LINUX_LIBRARIES)
    # Check if embedded libraries header exists
    set(EMBEDDED_LINUX_LIBS_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/export/embedded_linux_libraries.h")

    if(EXISTS "${EMBEDDED_LINUX_LIBS_HEADER}")
        add_definitions(-DLUPINE_EMBED_LINUX_LIBRARIES)
        message(STATUS "✓ Linux libraries will be embedded for cross-platform export")
    else()
        message(STATUS "⚠ Embedded Linux libraries header not found: ${EMBEDDED_LINUX_LIBS_HEADER}")
        message(STATUS "  Run: python scripts/generate_embedded_libraries.py --platform linux")
        message(STATUS "  Or disable with: -DLUPINE_EMBED_LINUX_LIBRARIES=OFF")
    endif()
else()
    message(STATUS "✗ Linux library embedding disabled")
endif()

# Configure Windows library embedding
if(LUPINE_EMBED_WINDOWS_LIBRARIES)
    # Check if embedded libraries header exists
    set(EMBEDDED_WINDOWS_LIBS_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/export/embedded_windows_libraries.h")

    if(EXISTS "${EMBEDDED_WINDOWS_LIBS_HEADER}")
        add_definitions(-DLUPINE_EMBED_WINDOWS_LIBRARIES)
        message(STATUS "✓ Windows libraries will be embedded for cross-platform export")
    else()
        message(STATUS "⚠ Embedded Windows libraries header not found: ${EMBEDDED_WINDOWS_LIBS_HEADER}")
        message(STATUS "  Run: python scripts/generate_embedded_libraries.py --platform windows")
        message(STATUS "  Or disable with: -DLUPINE_EMBED_WINDOWS_LIBRARIES=OFF")
    endif()
else()
    message(STATUS "✗ Windows library embedding disabled")
endif()

# Try to find Qt6
find_package(Qt6 QUIET COMPONENTS Core Widgets Gui OpenGL OpenGLWidgets)

# If Qt6 was not found via find_package, create tool targets manually
if(NOT TARGET Qt6::moc)
    add_executable(Qt6::moc IMPORTED)
    set_target_properties(Qt6::moc PROPERTIES IMPORTED_LOCATION "${QT_MOC_EXECUTABLE}")
endif()

if(NOT TARGET Qt6::uic)
    add_executable(Qt6::uic IMPORTED)
    set_target_properties(Qt6::uic PROPERTIES IMPORTED_LOCATION "${QT_UIC_EXECUTABLE}")
endif()

if(NOT TARGET Qt6::rcc)
    add_executable(Qt6::rcc IMPORTED)
    set_target_properties(Qt6::rcc PROPERTIES IMPORTED_LOCATION "${QT_RCC_EXECUTABLE}")
endif()

# If not found, set Qt6_FOUND to true and configure manually
if(NOT Qt6_FOUND)
    set(Qt6_FOUND TRUE)
    message(STATUS "Qt6 not found via find_package, configuring manually")
endif()

# Include directories
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${THIRDPARTY_DIR}/Sol"
    "${THIRDPARTY_DIR}/pybind11_x64-windows/include"
    "${THIRDPARTY_DIR}/lua_x64-windows-static/include"
    "${THIRDPARTY_DIR}/lua_x64-windows/include"
    "${THIRDPARTY_DIR}/python3_x64-windows-static/include"
    "${THIRDPARTY_DIR}/python3_x64-windows/include"
    "${THIRDPARTY_DIR}/python3_x64-windows-static/include/python3.12"
    "${THIRDPARTY_DIR}/python3_x64-windows/include/python3.12"
    "${THIRDPARTY_DIR}/glad_x64-windows/include"
    "${THIRDPARTY_DIR}/stb_x64-windows/include"
    "${THIRDPARTY_DIR}/boost-headers_x64-windows/include"
    "${THIRDPARTY_DIR}/boost-config_x64-windows/include"
    "${THIRDPARTY_DIR}/boost-uuid_x64-windows/include"
    "${THIRDPARTY_DIR}/boost-type-traits_x64-windows/include"
    "${THIRDPARTY_DIR}/boost-assert_x64-windows/include"
    "${THIRDPARTY_DIR}/boost-static-assert_x64-windows/include"
    "${THIRDPARTY_DIR}/boost-throw-exception_x64-windows/include"
    "${THIRDPARTY_DIR}/glm_x64-windows/include"
    "${THIRDPARTY_DIR}/assimp_x64-windows-static/include"
    "${THIRDPARTY_DIR}/assimp_x64-windows/include"
    "${THIRDPARTY_DIR}/yaml-cpp_x64-windows/include"
    "${THIRDPARTY_DIR}/nlohmann-json_x64-windows/include"
    "${THIRDPARTY_DIR}/spdlog_x64-windows/include"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows-static/include"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows/include"
    "${THIRDPARTY_DIR}/sdl2_x64-windows-static/include"
    "${THIRDPARTY_DIR}/sdl2_x64-windows-static/include/SDL2"
    "${THIRDPARTY_DIR}/sdl2_x64-windows/include"
    "${THIRDPARTY_DIR}/sdl2_x64-windows/include/SDL2"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows-static/include"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows-static/include/SDL2"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows/include"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows/include/SDL2"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows-static/include"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows-static/include/SDL2"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows/include"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows/include/SDL2"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows-static/include"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows-static/include/SDL2"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows/include"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows/include/SDL2"
    "${THIRDPARTY_DIR}/libstk_x64-windows-static/include"
    "${THIRDPARTY_DIR}/libstk_x64-windows/include"
    "${THIRDPARTY_DIR}/libvorbis_x64-windows-static/include"
    "${THIRDPARTY_DIR}/libvorbis_x64-windows/include"
    "${THIRDPARTY_DIR}/libogg_x64-windows-static/include"
    "${THIRDPARTY_DIR}/libogg_x64-windows/include"
    "${THIRDPARTY_DIR}/libflac_x64-windows-static/include"
    "${THIRDPARTY_DIR}/libflac_x64-windows/include"
    "${THIRDPARTY_DIR}/mp3lame_x64-windows-static/include"
    "${THIRDPARTY_DIR}/mp3lame_x64-windows/include"
    "${THIRDPARTY_DIR}/mpg123_x64-windows-static/include"
    "${THIRDPARTY_DIR}/mpg123_x64-windows/include"
    "${THIRDPARTY_DIR}/opus_x64-windows-static/include"
    "${THIRDPARTY_DIR}/opus_x64-windows/include"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows/include"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows/include"
    "${THIRDPARTY_DIR}/qtbase_x64-windows-static/include"
    "${THIRDPARTY_DIR}/qtbase_x64-windows-static/include/QtCore"
    "${THIRDPARTY_DIR}/qtbase_x64-windows-static/include/QtWidgets"
    "${THIRDPARTY_DIR}/qtbase_x64-windows-static/include/QtGui"
    "${THIRDPARTY_DIR}/qtbase_x64-windows-static/include/QtOpenGL"
    "${THIRDPARTY_DIR}/qtbase_x64-windows-static/include/QtOpenGLWidgets"
    "${THIRDPARTY_DIR}/qtbase_x64-windows/include"
    "${THIRDPARTY_DIR}/qtbase_x64-windows/include/QtCore"
    "${THIRDPARTY_DIR}/qtbase_x64-windows/include/QtWidgets"
    "${THIRDPARTY_DIR}/qtbase_x64-windows/include/QtGui"
    "${THIRDPARTY_DIR}/qtbase_x64-windows/include/QtOpenGL"
    "${THIRDPARTY_DIR}/qtbase_x64-windows/include/QtOpenGLWidgets"
    "${THIRDPARTY_DIR}/glad_x64-windows-static/include"
    "${THIRDPARTY_DIR}/glad_x64-windows/include"
    "${THIRDPARTY_DIR}/opengl_x64-windows/include"
    "${THIRDPARTY_DIR}/box2D_x64-windows-static/include"
    "${THIRDPARTY_DIR}/box2D/include"
    "${THIRDPARTY_DIR}/bullet3_x64-windows-static/include"
    "${THIRDPARTY_DIR}/bullet3_x64-windows/include"
    "${THIRDPARTY_DIR}/polyclipping_x64-windows-static/include"
    "${THIRDPARTY_DIR}/polyclipping_x64-windows/include"
    "${THIRDPARTY_DIR}/minizip_x64-windows-static/include"
    "${THIRDPARTY_DIR}/minizip_x64-windows/include"
    "${THIRDPARTY_DIR}/jhasse-poly2tri_x64-windows-static/include"
    "${THIRDPARTY_DIR}/jhasse-poly2tri_x64-windows/include"
    "${THIRDPARTY_DIR}/pugixml_x64-windows-static/include"
    "${THIRDPARTY_DIR}/pugixml_x64-windows/include"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows-static/include"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows/include"
    "${THIRDPARTY_DIR}/stb_x64-windows-static/include"
    "${THIRDPARTY_DIR}/stb_x64-windows/include"
    "${THIRDPARTY_DIR}/harfbuzz_x64-windows-static/include"
    "${THIRDPARTY_DIR}/harfbuzz_x64-windows/include"
    "${THIRDPARTY_DIR}/icu_x64-windows-static/include"
    "${THIRDPARTY_DIR}/icu_x64-windows/include"
    "${THIRDPARTY_DIR}/pcre2_x64-windows-static/include"
    "${THIRDPARTY_DIR}/pcre2_x64-windows/include"
    "${THIRDPARTY_DIR}/zstd_x64-windows-static/include"
    "${THIRDPARTY_DIR}/zstd_x64-windows/include"
    "${THIRDPARTY_DIR}/freetype_x64-windows-static/include"
    "${THIRDPARTY_DIR}/freetype_x64-windows/include"
    "${THIRDPARTY_DIR}/libpng_x64-windows-static/include"
    "${THIRDPARTY_DIR}/libpng_x64-windows/include"
    "${THIRDPARTY_DIR}/zlib_x64-windows-static/include"
    "${THIRDPARTY_DIR}/zlib_x64-windows/include"
    "${THIRDPARTY_DIR}/brotli_x64-windows-static/include"
    "${THIRDPARTY_DIR}/brotli_x64-windows/include"
    "${THIRDPARTY_DIR}/double-conversion_x64-windows-static/include"
    "${THIRDPARTY_DIR}/double-conversion_x64-windows/include"
)

# Source files
set(ENGINE_SOURCES
    src/core/UUID.cpp
    src/core/Node.cpp
    src/core/Component.cpp
    src/core/ComponentRegistration.cpp
    src/core/GlobalsManager.cpp
    src/core/Scene.cpp
    src/core/Project.cpp
    src/core/Engine.cpp
    src/core/CrashHandler.cpp
    src/nodes/Node2D.cpp
    src/nodes/Node3D.cpp
    src/nodes/Control.cpp
    src/components/Sprite2D.cpp
    src/components/Label.cpp
    src/components/PrimitiveMesh.cpp
    src/components/StaticMesh.cpp
    src/components/SkinnedMesh.cpp
    src/components/Button.cpp
    src/components/Panel.cpp
    src/components/TextureRectangle.cpp
    src/components/ColorRectangle.cpp
    src/components/NinePatchPanel.cpp
    src/components/ProgressBar.cpp
    src/components/Sprite3D.cpp
    src/components/Skybox3D.cpp
    src/components/OmniLight.cpp
    src/components/DirectionalLight.cpp
    src/components/SpotLight.cpp
    src/components/AudioSource.cpp
    src/components/Camera2D.cpp
    src/components/Camera3D.cpp
    src/components/Animator.cpp
    src/components/StateAnimator.cpp
    src/components/AnimatedSprite2D.cpp
    src/components/AnimatedSprite3D.cpp
    src/components/Tilemap2D.cpp
    src/components/Tilemap25D.cpp
    src/components/Tilemap3D.cpp
    src/components/AnimationPlayer.cpp
    src/animation/PropertySystem.cpp
    src/animation/PropertyStateManager.cpp
    src/animation/PropertyInterpolator.cpp
    src/animation/AutokeySystem.cpp
    src/rendering/Renderer.cpp
    src/rendering/TextRenderer.cpp
    src/rendering/Camera.cpp
    src/rendering/GridRenderer.cpp
    src/rendering/ViewportManager.cpp
    src/rendering/DebugRenderer.cpp
    src/rendering/LightingSystem.cpp
    src/resources/ResourceManager.cpp
    src/resources/MeshLoader.cpp
    src/resources/AnimationResource.cpp
    src/resources/StateMachineResource.cpp
    src/resources/TilesetResource.cpp
    src/resources/Tileset3DResource.cpp
    src/tilemap/TilemapData.cpp
    src/scripting/LuaScriptComponent.cpp
    src/scripting/PythonScriptComponent.cpp
    src/scripting/VisualScriptComponent.cpp
    # Scriptable Objects
    src/scriptableobjects/ScriptableObjectTemplate.cpp
    src/scriptableobjects/ScriptableObjectInstance.cpp
    src/scriptableobjects/ScriptableObjectManager.cpp
    src/scriptableobjects/ScriptableObjectBindings.cpp
    src/visualscripting/VScriptGraph.cpp
    src/visualscripting/VScriptNode.cpp
    src/visualscripting/VScriptConnection.cpp
    src/visualscripting/VScriptNodeTypes.cpp
    src/visualscripting/CodeGenerator.cpp
    src/input/InputManager.cpp
    src/input/InputConstants.cpp
    src/input/ActionMap.cpp
    src/components/PlayerController2D.cpp
    src/components/PlayerController3D.cpp
    src/components/PlatformerController2D.cpp
    src/components/PlayerController25D.cpp
    src/audio/AudioManager.cpp
    src/physics/PhysicsManager.cpp
    src/components/RigidBody2D.cpp
    src/components/RigidBody3D.cpp
    src/components/CollisionShape2D.cpp
    src/components/CollisionPolygon2D.cpp
    src/components/CollisionMesh3D.cpp
    src/components/Area2D.cpp
    src/components/Area3D.cpp
    src/components/KinematicBody2D.cpp
    src/components/KinematicBody3D.cpp
    src/components/TerrainRenderer.cpp
    src/components/TerrainCollider.cpp
    src/components/TerrainLoader.cpp
    src/terrain/TerrainData.cpp
    src/terrain/TerrainBrush.cpp
    src/terrain/TextureBrush.cpp
    src/terrain/AssetScatterBrush.cpp
    src/serialization/SceneSerializer.cpp
    src/serialization/ProjectSerializer.cpp
    src/serialization/SerializationUtils.cpp
    src/serialization/JsonUtils.cpp
    src/localization/LocalizationManager.cpp
    src/localization/LocalizationAPI.cpp
    src/export/ExportManager.cpp
    src/export/WindowsExporter.cpp
    src/export/LinuxExporter.cpp
    src/export/LinuxPackageBuilder.cpp
    src/export/EmbeddedLibraryManager.cpp
    src/export/WebExporter.cpp
    src/export/AssetBundler.cpp
    ${THIRDPARTY_DIR}/glad_x64-windows/include/glad/glad.c
)

# Header files
set(ENGINE_HEADERS
    include/lupine/core/UUID.h
    include/lupine/core/Node.h
    include/lupine/core/Component.h
    include/lupine/core/Scene.h
    include/lupine/core/Project.h
    include/lupine/core/Engine.h
    include/lupine/nodes/Node2D.h
    include/lupine/nodes/Node3D.h
    include/lupine/nodes/Control.h
    include/lupine/components/Sprite2D.h
    include/lupine/components/Label.h
    include/lupine/components/PrimitiveMesh.h
    include/lupine/components/StaticMesh.h
    include/lupine/components/SkinnedMesh.h
    include/lupine/components/Button.h
    include/lupine/components/Panel.h
    include/lupine/components/TextureRectangle.h
    include/lupine/components/ColorRectangle.h
    include/lupine/components/NinePatchPanel.h
    include/lupine/components/ProgressBar.h
    include/lupine/components/Sprite3D.h
    include/lupine/components/Skybox3D.h
    include/lupine/components/OmniLight.h
    include/lupine/components/DirectionalLight.h
    include/lupine/components/SpotLight.h
    include/lupine/components/AudioSource.h
    include/lupine/components/Camera2D.h
    include/lupine/components/Camera3D.h
    include/lupine/components/Animator.h
    include/lupine/components/StateAnimator.h
    include/lupine/components/AnimatedSprite2D.h
    include/lupine/components/AnimatedSprite3D.h
    include/lupine/components/Tilemap2D.h
    include/lupine/components/Tilemap25D.h
    include/lupine/components/Tilemap3D.h
    include/lupine/components/AnimationPlayer.h

    include/lupine/animation/PropertySystem.h
    include/lupine/animation/AutokeySystem.h
    include/lupine/rendering/Renderer.h
    include/lupine/rendering/Camera.h
    include/lupine/rendering/ViewportManager.h
    include/lupine/rendering/DebugRenderer.h
    include/lupine/rendering/LightingSystem.h
    include/lupine/resources/ResourceManager.h
    include/lupine/resources/MeshLoader.h
    include/lupine/resources/AnimationResource.h
    include/lupine/resources/StateMachineResource.h
    include/lupine/scripting/LuaScriptComponent.h
    include/lupine/scripting/PythonScriptComponent.h
    include/lupine/scripting/VisualScriptComponent.h
    # Scriptable Objects
    include/lupine/scriptableobjects/ScriptableObjectTemplate.h
    include/lupine/scriptableobjects/ScriptableObjectInstance.h
    include/lupine/scriptableobjects/ScriptableObjectManager.h
    include/lupine/scriptableobjects/ScriptableObjectBindings.h
    include/lupine/visualscripting/VScriptGraph.h
    include/lupine/visualscripting/VScriptNode.h
    include/lupine/visualscripting/VScriptConnection.h
    include/lupine/visualscripting/VScriptNodeTypes.h
    include/lupine/visualscripting/CodeGenerator.h
    include/lupine/input/InputManager.h
    include/lupine/input/InputConstants.h
    include/lupine/input/ActionMap.h
    include/lupine/components/PlayerController2D.h
    include/lupine/components/PlayerController3D.h
    include/lupine/components/PlatformerController2D.h
    include/lupine/audio/AudioManager.h
    include/lupine/physics/PhysicsManager.h
    include/lupine/components/RigidBody2D.h
    include/lupine/components/RigidBody3D.h
    include/lupine/components/CollisionShape2D.h
    include/lupine/components/CollisionPolygon2D.h
    include/lupine/components/CollisionMesh3D.h
    include/lupine/components/Area2D.h
    include/lupine/components/Area3D.h
    include/lupine/components/KinematicBody2D.h
    include/lupine/components/KinematicBody3D.h
    include/lupine/serialization/SerializationUtils.h
    include/lupine/serialization/SceneSerializer.h
    include/lupine/serialization/ProjectSerializer.h
    include/lupine/export/ExportManager.h
    include/lupine/export/WindowsExporter.h
    include/lupine/export/LinuxExporter.h
    include/lupine/export/LinuxPackageBuilder.h
    include/lupine/export/EmbeddedLibraryManager.h
    include/lupine/export/WebExporter.h
    include/lupine/export/AssetBundler.h
    include/lupine/Lupine.h
)

# Create engine library
add_library(LupineEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# Add library directories (prefer static versions)
link_directories(
    "${THIRDPARTY_DIR}/boost-uuid_x64-windows/lib"
    "${THIRDPARTY_DIR}/assimp_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/assimp_x64-windows/lib"
    "${THIRDPARTY_DIR}/yaml-cpp_x64-windows/lib"
    "${THIRDPARTY_DIR}/spdlog_x64-windows/lib"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows/lib"
    "${THIRDPARTY_DIR}/minizip_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/minizip_x64-windows/lib"
    "${THIRDPARTY_DIR}/sdl2_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libstk_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libvorbis_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libogg_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libflac_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/mp3lame_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/mpg123_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/opus_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/sdl2_x64-windows/lib"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows/lib"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows/lib"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows/lib"
    "${THIRDPARTY_DIR}/libstk_x64-windows/lib"
    "${THIRDPARTY_DIR}/libvorbis_x64-windows/lib"
    "${THIRDPARTY_DIR}/libogg_x64-windows/lib"
    "${THIRDPARTY_DIR}/libflac_x64-windows/lib"
    "${THIRDPARTY_DIR}/mp3lame_x64-windows/lib"
    "${THIRDPARTY_DIR}/mpg123_x64-windows/lib"
    "${THIRDPARTY_DIR}/opus_x64-windows/lib"
    "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/qtbase_x64-windows/lib"
    "${THIRDPARTY_DIR}/glad_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/glad_x64-windows/lib"
    "${THIRDPARTY_DIR}/opengl_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/opengl_x64-windows/lib"
    "${THIRDPARTY_DIR}/lua_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/lua_x64-windows/lib"
    "${THIRDPARTY_DIR}/python3_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/python3_x64-windows/lib"
    "${THIRDPARTY_DIR}/box2D_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/box2D/lib"
    "${THIRDPARTY_DIR}/bullet3_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/bullet3_x64-windows/lib"
    "${THIRDPARTY_DIR}/polyclipping_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/polyclipping_x64-windows/lib"
    "${THIRDPARTY_DIR}/harfbuzz_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/harfbuzz_x64-windows/lib"
    "${THIRDPARTY_DIR}/icu_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/icu_x64-windows/lib"
    "${THIRDPARTY_DIR}/pcre2_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/pcre2_x64-windows/lib"
    "${THIRDPARTY_DIR}/zstd_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/zstd_x64-windows/lib"
    "${THIRDPARTY_DIR}/freetype_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/freetype_x64-windows/lib"
    "${THIRDPARTY_DIR}/libpng_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libpng_x64-windows/lib"
    "${THIRDPARTY_DIR}/zlib_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/zlib_x64-windows/lib"
    "${THIRDPARTY_DIR}/brotli_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/brotli_x64-windows/lib"
    "${THIRDPARTY_DIR}/double-conversion_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/double-conversion_x64-windows/lib"
    "${THIRDPARTY_DIR}/jhasse-poly2tri_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/jhasse-poly2tri_x64-windows/lib"
)

# Function to find static library first, then fall back to dynamic
function(find_static_or_dynamic_library VAR_NAME LIB_NAME SEARCH_PATHS)
    set(STATIC_LIB_FOUND FALSE)

    # Try to find static library (.lib) first
    foreach(SEARCH_PATH ${SEARCH_PATHS})
        if(EXISTS "${SEARCH_PATH}/${LIB_NAME}.lib")
            set(${VAR_NAME} "${SEARCH_PATH}/${LIB_NAME}.lib" PARENT_SCOPE)
            set(STATIC_LIB_FOUND TRUE)
            message(STATUS "✓ Static linking: ${LIB_NAME}.lib")
            break()
        endif()
    endforeach()

    # If static library not found, fall back to dynamic linking (library name only)
    if(NOT STATIC_LIB_FOUND)
        set(${VAR_NAME} ${LIB_NAME} PARENT_SCOPE)
        message(STATUS "⚠ Dynamic linking: ${LIB_NAME} (static .lib not found)")
    endif()
endfunction()

message(STATUS "=== Lupine Engine Library Linking Configuration ===")
message(STATUS "Prioritizing static libraries (.lib) over dynamic linking...")
message(STATUS "")

# Define library search paths (prefer static versions)
set(LIB_SEARCH_PATHS
    "${THIRDPARTY_DIR}/sdl2_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/sdl2_x64-windows/lib"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/sdl2-image_x64-windows/lib"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows/lib"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows/lib"
    "${THIRDPARTY_DIR}/libstk_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libstk_x64-windows/lib"
    "${THIRDPARTY_DIR}/libvorbis_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libvorbis_x64-windows/lib"
    "${THIRDPARTY_DIR}/libogg_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libogg_x64-windows/lib"
    "${THIRDPARTY_DIR}/libflac_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libflac_x64-windows/lib"
    "${THIRDPARTY_DIR}/mp3lame_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/mp3lame_x64-windows/lib"
    "${THIRDPARTY_DIR}/mpg123_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/mpg123_x64-windows/lib"
    "${THIRDPARTY_DIR}/opus_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/opus_x64-windows/lib"
    "${THIRDPARTY_DIR}/assimp_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/assimp_x64-windows/lib"
    "${THIRDPARTY_DIR}/lua_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/lua_x64-windows/lib"
    "${THIRDPARTY_DIR}/python3_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/python3_x64-windows/lib"
    "${THIRDPARTY_DIR}/glad_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/glad_x64-windows/lib"
    "${THIRDPARTY_DIR}/box2D_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/box2D/lib"
    "${THIRDPARTY_DIR}/bullet3_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/bullet3_x64-windows/lib"
    "${THIRDPARTY_DIR}/polyclipping_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/polyclipping_x64-windows/lib"
    "${THIRDPARTY_DIR}/minizip_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/minizip_x64-windows/lib"
    "${THIRDPARTY_DIR}/pugixml_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/pugixml_x64-windows/lib"
    "${THIRDPARTY_DIR}/yaml-cpp_x64-windows/lib"
    "${THIRDPARTY_DIR}/spdlog_x64-windows/lib"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libsndfile_x64-windows/lib"
    "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/qtbase_x64-windows/lib"
    "${THIRDPARTY_DIR}/harfbuzz_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/harfbuzz_x64-windows/lib"
    "${THIRDPARTY_DIR}/icu_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/icu_x64-windows/lib"
    "${THIRDPARTY_DIR}/pcre2_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/pcre2_x64-windows/lib"
    "${THIRDPARTY_DIR}/zstd_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/zstd_x64-windows/lib"
    "${THIRDPARTY_DIR}/freetype_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/freetype_x64-windows/lib"
    "${THIRDPARTY_DIR}/libpng_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/libpng_x64-windows/lib"
    "${THIRDPARTY_DIR}/zlib_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/zlib_x64-windows/lib"
    "${THIRDPARTY_DIR}/brotli_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/brotli_x64-windows/lib"
    "${THIRDPARTY_DIR}/double-conversion_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/double-conversion_x64-windows/lib"
    "${THIRDPARTY_DIR}/jhasse-poly2tri_x64-windows-static/lib"
    "${THIRDPARTY_DIR}/jhasse-poly2tri_x64-windows/lib"
)

# Find libraries (static first, then dynamic fallback)
find_static_or_dynamic_library(LUA_LIB "lua" "${THIRDPARTY_DIR}/lua_x64-windows-static/lib;${THIRDPARTY_DIR}/lua_x64-windows/lib")
find_static_or_dynamic_library(PYTHON312_LIB "python312" "${THIRDPARTY_DIR}/python3_x64-windows-static/lib;${THIRDPARTY_DIR}/python3_x64-windows/lib")
# SDL2 static libraries have different names
find_static_or_dynamic_library(SDL2_LIB "SDL2-static" "${THIRDPARTY_DIR}/sdl2_x64-windows-static/lib;${THIRDPARTY_DIR}/sdl2_x64-windows/lib")
find_static_or_dynamic_library(SDL2_MAIN_LIB "SDL2main" "${THIRDPARTY_DIR}/sdl2_x64-windows-static/lib/manual-link;${THIRDPARTY_DIR}/sdl2_x64-windows/lib")
find_static_or_dynamic_library(SDL2_IMAGE_LIB "SDL2_image" "${THIRDPARTY_DIR}/sdl2-image_x64-windows-static/lib;${THIRDPARTY_DIR}/sdl2-image_x64-windows/lib")
find_static_or_dynamic_library(SDL2_TTF_LIB "SDL2_ttf" "${THIRDPARTY_DIR}/sdl2-ttf_x64-windows-static/lib;${THIRDPARTY_DIR}/sdl2-ttf_x64-windows/lib")
find_static_or_dynamic_library(SDL2_MIXER_LIB "SDL2_mixer" "${THIRDPARTY_DIR}/sdl2-mixer_x64-windows-static/lib;${THIRDPARTY_DIR}/sdl2-mixer_x64-windows/lib")
find_static_or_dynamic_library(STK_LIB "libstk" "${THIRDPARTY_DIR}/libstk_x64-windows-static/lib;${THIRDPARTY_DIR}/libstk_x64-windows/lib")
find_static_or_dynamic_library(VORBIS_LIB "vorbis" "${THIRDPARTY_DIR}/libvorbis_x64-windows-static/lib;${THIRDPARTY_DIR}/libvorbis_x64-windows/lib")
find_static_or_dynamic_library(VORBISFILE_LIB "vorbisfile" "${THIRDPARTY_DIR}/libvorbis_x64-windows-static/lib;${THIRDPARTY_DIR}/libvorbis_x64-windows/lib")
find_static_or_dynamic_library(OGG_LIB "ogg" "${THIRDPARTY_DIR}/libogg_x64-windows-static/lib;${THIRDPARTY_DIR}/libogg_x64-windows/lib")
find_static_or_dynamic_library(FLAC_LIB "FLAC" "${THIRDPARTY_DIR}/libflac_x64-windows-static/lib;${THIRDPARTY_DIR}/libflac_x64-windows/lib")
find_static_or_dynamic_library(MP3LAME_LIB "libmp3lame-static" "${THIRDPARTY_DIR}/mp3lame_x64-windows-static/lib;${THIRDPARTY_DIR}/mp3lame_x64-windows/lib")
find_static_or_dynamic_library(MPGHIP_LIB "libmpghip-static" "${THIRDPARTY_DIR}/mp3lame_x64-windows-static/lib;${THIRDPARTY_DIR}/mp3lame_x64-windows/lib")
find_static_or_dynamic_library(MPG123_LIB "mpg123" "${THIRDPARTY_DIR}/mpg123_x64-windows-static/lib;${THIRDPARTY_DIR}/mpg123_x64-windows/lib")
find_static_or_dynamic_library(OPUS_LIB "opus" "${THIRDPARTY_DIR}/opus_x64-windows-static/lib;${THIRDPARTY_DIR}/opus_x64-windows/lib")
find_static_or_dynamic_library(ASSIMP_LIB "assimp-vc143-mt" "${THIRDPARTY_DIR}/assimp_x64-windows-static/lib;${THIRDPARTY_DIR}/assimp_x64-windows/lib")
find_static_or_dynamic_library(GLAD_LIB "glad" "${THIRDPARTY_DIR}/glad_x64-windows-static/lib;${THIRDPARTY_DIR}/glad_x64-windows/lib")
find_static_or_dynamic_library(BULLET_DYNAMICS_LIB "BulletDynamics" "${THIRDPARTY_DIR}/bullet3_x64-windows-static/lib;${THIRDPARTY_DIR}/bullet3_x64-windows/lib")
find_static_or_dynamic_library(BULLET_COLLISION_LIB "BulletCollision" "${THIRDPARTY_DIR}/bullet3_x64-windows-static/lib;${THIRDPARTY_DIR}/bullet3_x64-windows/lib")
find_static_or_dynamic_library(LINEAR_MATH_LIB "LinearMath" "${THIRDPARTY_DIR}/bullet3_x64-windows-static/lib;${THIRDPARTY_DIR}/bullet3_x64-windows/lib")
find_static_or_dynamic_library(YAML_CPP_LIB "yaml-cpp" "${THIRDPARTY_DIR}/yaml-cpp_x64-windows/lib")
find_static_or_dynamic_library(SPDLOG_LIB "spdlog" "${THIRDPARTY_DIR}/spdlog_x64-windows/lib")
find_static_or_dynamic_library(SNDFILE_LIB "sndfile" "${THIRDPARTY_DIR}/libsndfile_x64-windows-static/lib;${THIRDPARTY_DIR}/libsndfile_x64-windows/lib")

# Handle Box2D with debug/release variants
find_static_or_dynamic_library(BOX2D_DEBUG_LIB "box2dd" "${THIRDPARTY_DIR}/box2D_x64-windows-static/lib;${THIRDPARTY_DIR}/box2D/lib")
find_static_or_dynamic_library(BOX2D_RELEASE_LIB "box2d" "${THIRDPARTY_DIR}/box2D_x64-windows-static/lib;${THIRDPARTY_DIR}/box2D/lib")

# Find polyclipping library
find_static_or_dynamic_library(POLYCLIPPING_LIB "polyclipping" "${THIRDPARTY_DIR}/polyclipping_x64-windows-static/lib;${THIRDPARTY_DIR}/polyclipping_x64-windows/lib")

# Find additional libraries
find_static_or_dynamic_library(MINIZIP_LIB "minizip" "${THIRDPARTY_DIR}/minizip_x64-windows-static/lib;${THIRDPARTY_DIR}/minizip_x64-windows/lib")
find_static_or_dynamic_library(POLY2TRI_LIB "poly2tri" "${THIRDPARTY_DIR}/jhasse-poly2tri_x64-windows-static/lib;${THIRDPARTY_DIR}/jhasse-poly2tri_x64-windows/lib")
find_static_or_dynamic_library(PUGIXML_LIB "pugixml" "${THIRDPARTY_DIR}/pugixml_x64-windows-static/lib;${THIRDPARTY_DIR}/pugixml_x64-windows/lib")

# Find missing static libraries that are causing DLL dependencies
find_static_or_dynamic_library(BZIP2_LIB "bz2" "${THIRDPARTY_DIR}/bzip2_x64-windows-static/lib;${THIRDPARTY_DIR}/bzip2_x64-windows/lib")
find_static_or_dynamic_library(LIBJPEG_TURBO_LIB "jpeg" "${THIRDPARTY_DIR}/libjpeg-turbo_x64-windows-static/lib;${THIRDPARTY_DIR}/libjpeg-turbo_x64-windows/lib")
find_static_or_dynamic_library(TURBOJPEG_LIB "turbojpeg" "${THIRDPARTY_DIR}/libjpeg-turbo_x64-windows-static/lib;${THIRDPARTY_DIR}/libjpeg-turbo_x64-windows/lib")
find_static_or_dynamic_library(LZMA_LIB "lzma" "${THIRDPARTY_DIR}/liblzma_x64-windows/lib")
find_static_or_dynamic_library(LZ4_LIB "lz4" "${THIRDPARTY_DIR}/lz4_x64-windows-static/lib;${THIRDPARTY_DIR}/lz4_x64-windows/lib")
find_static_or_dynamic_library(WAVPACK_LIB "libwavpack" "${THIRDPARTY_DIR}/wavpack_x64-windows-static/lib;${THIRDPARTY_DIR}/wavpack_x64-windows/lib")
find_static_or_dynamic_library(SQLITE3_LIB "sqlite3" "${THIRDPARTY_DIR}/sqlite3_x64-windows-static/lib;${THIRDPARTY_DIR}/sqlite3_x64-windows/lib")
find_static_or_dynamic_library(OPENSSL_SSL_LIB "libssl" "${THIRDPARTY_DIR}/openssl_x64-windows-static/lib;${THIRDPARTY_DIR}/openssl_x64-windows/lib")
find_static_or_dynamic_library(OPENSSL_CRYPTO_LIB "libcrypto" "${THIRDPARTY_DIR}/openssl_x64-windows-static/lib;${THIRDPARTY_DIR}/openssl_x64-windows/lib")

# Find additional static libraries required for SDL2_ttf and other dependencies
find_static_or_dynamic_library(FREETYPE_LIB "freetype" "${THIRDPARTY_DIR}/freetype_x64-windows-static/lib;${THIRDPARTY_DIR}/freetype_x64-windows/lib")
find_static_or_dynamic_library(LIBPNG_LIB "libpng16" "${THIRDPARTY_DIR}/libpng_x64-windows-static/lib;${THIRDPARTY_DIR}/libpng_x64-windows/lib")
find_static_or_dynamic_library(ZLIB_LIB "zlib" "${THIRDPARTY_DIR}/zlib_x64-windows-static/lib;${THIRDPARTY_DIR}/zlib_x64-windows/lib")
find_static_or_dynamic_library(BROTLI_DEC_LIB "brotlidec" "${THIRDPARTY_DIR}/brotli_x64-windows-static/lib;${THIRDPARTY_DIR}/brotli_x64-windows/lib")
find_static_or_dynamic_library(BROTLI_COMMON_LIB "brotlicommon" "${THIRDPARTY_DIR}/brotli_x64-windows-static/lib;${THIRDPARTY_DIR}/brotli_x64-windows/lib")

# Find additional optional static libraries that may be dependencies
find_static_or_dynamic_library(KUBAZIP_LIB "kubazip" "${THIRDPARTY_DIR}/kubazip_x64-windows-static/lib;${THIRDPARTY_DIR}/kubazip_x64-windows/lib")
find_static_or_dynamic_library(DRACO_LIB "draco" "${THIRDPARTY_DIR}/draco_x64-windows-static/lib;${THIRDPARTY_DIR}/draco_x64-windows/lib")

# Link libraries to engine
target_link_libraries(LupineEngine
    ${LUA_LIB}
    ${PYTHON312_LIB}
    ${SDL2_LIB}
    ${SDL2_MAIN_LIB}
    ${SDL2_IMAGE_LIB}
    ${SDL2_TTF_LIB}
    ${SDL2_MIXER_LIB}
    ${STK_LIB}
    ${VORBIS_LIB}
    ${VORBISFILE_LIB}
    ${OGG_LIB}
    ${FLAC_LIB}
    ${MP3LAME_LIB}
    ${MPGHIP_LIB}
    ${MPG123_LIB}
    ${OPUS_LIB}
    ${ASSIMP_LIB}
    ${GLAD_LIB}
    ${YAML_CPP_LIB}
    ${SPDLOG_LIB}
    ${SNDFILE_LIB}
    # Additional static libraries to prevent DLL dependencies
    ${BZIP2_LIB}
    ${LIBJPEG_TURBO_LIB}
    ${TURBOJPEG_LIB}
    ${LZMA_LIB}
    ${LZ4_LIB}
    ${WAVPACK_LIB}
    ${SQLITE3_LIB}
    ${OPENSSL_SSL_LIB}
    ${OPENSSL_CRYPTO_LIB}
    # FreeType library required for SDL2_ttf static linking
    ${FREETYPE_LIB}
    ${LIBPNG_LIB}
    ${ZLIB_LIB}
    ${BROTLI_DEC_LIB}
    ${BROTLI_COMMON_LIB}
    # Optional additional libraries (may not be needed but available if required)
    ${KUBAZIP_LIB}
    ${DRACO_LIB}
    opengl32  # System library, always dynamic
    $<$<CONFIG:Debug>:${BOX2D_DEBUG_LIB}>
    $<$<CONFIG:Release>:${BOX2D_RELEASE_LIB}>
    ${BULLET_DYNAMICS_LIB}
    ${BULLET_COLLISION_LIB}
    ${LINEAR_MATH_LIB}
    ${POLYCLIPPING_LIB}
    ${MINIZIP_LIB}
    ${POLY2TRI_LIB}
    ${PUGIXML_LIB}
    # Windows system libraries required for SDL2 static linking
    winmm imm32 ole32 oleaut32 version setupapi
    # Additional Windows libraries for complete static linking
    user32 gdi32 shell32 advapi32 kernel32 ws2_32
)

message(STATUS "")
message(STATUS "=== Library Linking Complete ===")
message(STATUS "LupineEngine configured with static-first library linking")

# Add linker flags to handle runtime library conflicts and force static linking
if(WIN32)
    # For Release builds, exclude debug runtime and dynamic libraries
    target_link_options(LupineEngine PRIVATE
        $<$<CONFIG:Release>:/NODEFAULTLIB:MSVCRTD>
        $<$<CONFIG:Release>:/NODEFAULTLIB:msvcrt.dll>
        $<$<CONFIG:Release>:/NODEFAULTLIB:VCRUNTIME140.dll>
        $<$<CONFIG:Release>:/NODEFAULTLIB:VCRUNTIME140_1.dll>
    )
    # For Debug builds, exclude release runtime and dynamic libraries
    target_link_options(LupineEngine PRIVATE
        $<$<CONFIG:Debug>:/NODEFAULTLIB:MSVCRT>
        $<$<CONFIG:Debug>:/NODEFAULTLIB:msvcrtd.dll>
        $<$<CONFIG:Debug>:/NODEFAULTLIB:VCRUNTIME140.dll>
        $<$<CONFIG:Debug>:/NODEFAULTLIB:VCRUNTIME140_1.dll>
    )
    # Additional linker flags for complete static linking
    target_link_options(LupineEngine PRIVATE
        /SUBSYSTEM:CONSOLE
        /INCREMENTAL:NO
        # Force static linking of all libraries
        /NODEFAULTLIB:SDL2.dll
        /NODEFAULTLIB:freetype.dll
        /NODEFAULTLIB:zlib1.dll
        /NODEFAULTLIB:brotlidec.dll
        /NODEFAULTLIB:brotlicommon.dll
        /NODEFAULTLIB:libpng16.dll
        /NODEFAULTLIB:bz2.dll
    )
endif()



# Create CLI executable
set(CLI_SOURCES src/cli/main.cpp src/cli/CLICommands.cpp)
if(WIN32)
    set(CLI_SOURCES ${CLI_SOURCES} src/cli/resources/lupine-cli.rc)
endif()
add_executable(lupine-cli ${CLI_SOURCES})
target_link_libraries(lupine-cli LupineEngine)

# Set console subsystem for CLI
set_target_properties(lupine-cli PROPERTIES
    WIN32_EXECUTABLE FALSE
)

# Apply same linker options to CLI executable
if(WIN32)
    target_link_options(lupine-cli PRIVATE
        $<$<CONFIG:Debug>:/NODEFAULTLIB:MSVCRT>
        $<$<CONFIG:Release>:/NODEFAULTLIB:MSVCRTD>
        /SUBSYSTEM:CONSOLE
    )
endif()

# Create runtime executable
set(RUNTIME_SOURCES src/runtime/main.cpp)
if(WIN32)
    set(RUNTIME_SOURCES ${RUNTIME_SOURCES} src/runtime/resources/lupine-runtime.rc)
endif()
add_executable(lupine-runtime ${RUNTIME_SOURCES})
target_link_libraries(lupine-runtime LupineEngine)

# Set console subsystem for runtime
set_target_properties(lupine-runtime PROPERTIES
    WIN32_EXECUTABLE FALSE
)

# Apply same linker options to runtime executable
if(WIN32)
    target_link_options(lupine-runtime PRIVATE
        $<$<CONFIG:Debug>:/NODEFAULTLIB:MSVCRT>
        $<$<CONFIG:Release>:/NODEFAULTLIB:MSVCRTD>
        /SUBSYSTEM:CONSOLE
    )
endif()





# Create Qt editor executable (only if Qt6 is found)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    # Qt source files
    set(EDITOR_SOURCES
        src/editor/main.cpp
        src/editor/ProjectManager.cpp
        src/editor/MainWindow.cpp
        src/editor/IconManager.cpp
        src/editor/AssetDropHandler.cpp
        src/editor/PlacementMode.cpp
        src/editor/AssetPreviewModel.cpp
        src/editor/AssetTagManager.cpp
        src/editor/AssetListView.cpp
        src/editor/panels/SceneTreePanel.cpp
        src/editor/panels/AssetBrowserPanel.cpp
        src/editor/panels/FileBrowserPanel.cpp
        src/editor/panels/SceneViewPanel.cpp
        src/editor/panels/InspectorPanel.cpp
        src/editor/panels/ScriptEditorPanel.cpp
        src/editor/panels/ConsolePanel.cpp
        src/editor/panels/ActionMappingPanel.cpp
        src/editor/dialogs/ProjectSettingsDialog.cpp
        src/editor/widgets/GameRunnerToolbar.cpp
        src/editor/widgets/PropertyEditorWidget.cpp
        src/editor/dialogs/AddComponentDialog.cpp
        src/editor/dialogs/AddNodeDialog.cpp
        src/editor/dialogs/NodeSelectionDialog.cpp
        src/editor/dialogs/TweenAnimatorDialog.cpp

        src/editor/dialogs/SpriteAnimatorDialog.cpp
        src/editor/dialogs/StateAnimatorDialog.cpp
        src/editor/dialogs/TilesetEditorDialog.cpp
        src/editor/dialogs/Tileset3DEditorDialog.cpp
        src/editor/TileBuilderDialog.cpp
        src/editor/TileBuilderPreview.cpp
        src/editor/TileOBJExporter.cpp
        src/editor/TilePrimitiveMeshGenerator.cpp
        src/editor/TileTextureTemplateGenerator.cpp
        src/editor/TileTextureWatcher.cpp
        src/editor/dialogs/TilemapPainterDialog.cpp
        src/editor/dialogs/GlobalsManagerDialog.cpp
        src/editor/dialogs/ExportDialog.cpp
        src/editor/dialogs/PixelPainterDialog.cpp
        src/editor/dialogs/ScribblerDialog.cpp
        src/editor/dialogs/VoxelBlockerDialog.cpp
        src/editor/dialogs/TilemapBuilder3DDialog.cpp
        src/editor/dialogs/Tilemap25DPainterDialog.cpp
        src/editor/dialogs/VisualScripterDialog.cpp
        src/editor/dialogs/GraphCanvasWidget.cpp
        src/editor/dialogs/NotepadDialog.cpp
        src/editor/dialogs/TodoListDialog.cpp
        src/editor/dialogs/MilestoneTrackerDialog.cpp
        src/editor/dialogs/FeatureBugTrackerDialog.cpp
        src/editor/dialogs/AssetProgressTrackerDialog.cpp
        src/editor/dialogs/MenuBuilderDialog.cpp
        src/editor/dialogs/MenuSceneView.cpp
        src/editor/dialogs/LocalizationSettingsDialog.cpp
        src/editor/dialogs/LocalizationTablesDialog.cpp
        src/editor/dialogs/ScriptableObjectsDialog.cpp
        src/editor/dialogs/AudioMixerDialog.cpp
        src/editor/dialogs/TerrainPainterDialog.cpp
        src/editor/rendering/GizmoRenderer.cpp
        src/editor/SyntaxHighlighters.cpp
        src/editor/EditorUndoSystem.cpp
        src/editor/EditorClipboard.cpp
    )

    # Qt header files
    set(EDITOR_HEADERS
        src/editor/ProjectManager.h
        src/editor/MainWindow.h
        src/editor/IconManager.h
        src/editor/AssetDropHandler.h
        src/editor/PlacementMode.h
        src/editor/AssetPreviewModel.h
        src/editor/AssetTagManager.h
        src/editor/AssetListView.h
        src/editor/panels/SceneTreePanel.h
        src/editor/panels/AssetBrowserPanel.h
        src/editor/panels/FileBrowserPanel.h
        src/editor/panels/SceneViewPanel.h
        src/editor/panels/InspectorPanel.h
        src/editor/panels/ScriptEditorPanel.h
        src/editor/panels/ConsolePanel.h
        src/editor/panels/ActionMappingPanel.h
        src/editor/widgets/GameRunnerToolbar.h
        src/editor/widgets/PropertyEditorWidget.h
        src/editor/dialogs/AddComponentDialog.h
        src/editor/dialogs/AddNodeDialog.h
        src/editor/dialogs/NodeSelectionDialog.h
        src/editor/dialogs/ProjectSettingsDialog.h
        src/editor/dialogs/TweenAnimatorDialog.h
        src/editor/dialogs/SpriteAnimatorDialog.h
        src/editor/dialogs/StateAnimatorDialog.h
        src/editor/dialogs/TilesetEditorDialog.h
        src/editor/dialogs/Tileset3DEditorDialog.h
        include/lupine/editor/TileBuilderDialog.h
        include/lupine/editor/TileBuilder.h
        src/editor/dialogs/TilemapPainterDialog.h
        src/editor/dialogs/GlobalsManagerDialog.h
        src/editor/dialogs/ExportDialog.h
        src/editor/dialogs/PixelPainterDialog.h
        src/editor/dialogs/ScribblerDialog.h
        src/editor/dialogs/VoxelBlockerDialog.h
        src/editor/dialogs/TilemapBuilder3DDialog.h
        src/editor/dialogs/Tilemap25DPainterDialog.h
        src/editor/dialogs/VisualScripterDialog.h
        src/editor/dialogs/GraphCanvasWidget.h
        src/editor/dialogs/NotepadDialog.h
        src/editor/dialogs/TodoListDialog.h
        src/editor/dialogs/MilestoneTrackerDialog.h
        src/editor/dialogs/FeatureBugTrackerDialog.h
        src/editor/dialogs/AssetProgressTrackerDialog.h
        src/editor/dialogs/MenuBuilderDialog.h
        src/editor/dialogs/MenuSceneView.h
        src/editor/dialogs/ScriptableObjectsDialog.h
        src/editor/dialogs/AudioMixerDialog.h
        src/editor/dialogs/TerrainPainterDialog.h
        src/editor/rendering/GizmoRenderer.h
        src/editor/SyntaxHighlighters.h
        src/editor/EditorUndoSystem.h
        src/editor/EditorClipboard.h
    )

    # Qt resource files
    set(EDITOR_RESOURCES
        src/editor/resources/editor.qrc
    )

    # Windows resource file for icon
    if(WIN32)
        set(EDITOR_RESOURCES ${EDITOR_RESOURCES} src/editor/resources/lupine-editor.rc)
    endif()

    add_executable(lupine-editor ${EDITOR_SOURCES} ${EDITOR_HEADERS} ${EDITOR_RESOURCES})

    # Find Qt libraries (use static versions)
    find_static_or_dynamic_library(QT6_CORE_LIB "Qt6Core" "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib;${THIRDPARTY_DIR}/qtbase_x64-windows/lib")
    find_static_or_dynamic_library(QT6_WIDGETS_LIB "Qt6Widgets" "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib;${THIRDPARTY_DIR}/qtbase_x64-windows/lib")
    find_static_or_dynamic_library(QT6_GUI_LIB "Qt6Gui" "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib;${THIRDPARTY_DIR}/qtbase_x64-windows/lib")
    find_static_or_dynamic_library(QT6_OPENGL_LIB "Qt6OpenGL" "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib;${THIRDPARTY_DIR}/qtbase_x64-windows/lib")
    find_static_or_dynamic_library(QT6_OPENGL_WIDGETS_LIB "Qt6OpenGLWidgets" "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib;${THIRDPARTY_DIR}/qtbase_x64-windows/lib")
    find_static_or_dynamic_library(QT6_ENTRYPOINT_LIB "Qt6EntryPoint" "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib;${THIRDPARTY_DIR}/qtbase_x64-windows/lib")

    # Find Qt dependency libraries for static linking
    find_static_or_dynamic_library(HARFBUZZ_LIB "harfbuzz" "${THIRDPARTY_DIR}/harfbuzz_x64-windows-static/lib;${THIRDPARTY_DIR}/harfbuzz_x64-windows/lib")
    find_static_or_dynamic_library(ICU_UC_LIB "icuuc" "${THIRDPARTY_DIR}/icu_x64-windows-static/lib;${THIRDPARTY_DIR}/icu_x64-windows/lib")
    find_static_or_dynamic_library(ICU_IN_LIB "icuin" "${THIRDPARTY_DIR}/icu_x64-windows-static/lib;${THIRDPARTY_DIR}/icu_x64-windows/lib")
    find_static_or_dynamic_library(ICU_DT_LIB "icudt" "${THIRDPARTY_DIR}/icu_x64-windows-static/lib;${THIRDPARTY_DIR}/icu_x64-windows/lib")
    find_static_or_dynamic_library(PCRE2_16_LIB "pcre2-16" "${THIRDPARTY_DIR}/pcre2_x64-windows-static/lib;${THIRDPARTY_DIR}/pcre2_x64-windows/lib")
    find_static_or_dynamic_library(ZSTD_LIB "zstd" "${THIRDPARTY_DIR}/zstd_x64-windows-static/lib;${THIRDPARTY_DIR}/zstd_x64-windows/lib")
    find_static_or_dynamic_library(FREETYPE_LIB "freetype" "${THIRDPARTY_DIR}/freetype_x64-windows-static/lib")
    find_static_or_dynamic_library(LIBPNG_LIB "libpng16" "${THIRDPARTY_DIR}/libpng_x64-windows-static/lib")
    find_static_or_dynamic_library(ZLIB_LIB "zlib" "${THIRDPARTY_DIR}/zlib_x64-windows-static/lib")
    find_static_or_dynamic_library(BROTLI_DEC_LIB "brotlidec" "${THIRDPARTY_DIR}/brotli_x64-windows-static/lib")
    find_static_or_dynamic_library(BROTLI_COMMON_LIB "brotlicommon" "${THIRDPARTY_DIR}/brotli_x64-windows-static/lib")
    find_static_or_dynamic_library(DOUBLE_CONVERSION_LIB "double-conversion" "${THIRDPARTY_DIR}/double-conversion_x64-windows-static/lib")

    # Find Qt platform plugin libraries for static linking
    set(QT_PLATFORM_PLUGIN_LIB "${THIRDPARTY_DIR}/qtbase_x64-windows-static/Qt6/plugins/platforms/qwindows.lib")
    set(QT_GENERIC_PLUGIN_LIB "${THIRDPARTY_DIR}/qtbase_x64-windows-static/Qt6/plugins/generic/qtuiotouchplugin.lib")

    # Find Qt resource object files for static linking
    set(QT_GUI_SHADERS_OBJ "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib/objects-Release/Gui_resources_2/.qt/rcc/qrc_gui_shaders_init.cpp.obj")
    set(QT_PDF_INIT_OBJ "${THIRDPARTY_DIR}/qtbase_x64-windows-static/lib/objects-Release/Gui_resources_1/.qt/rcc/qrc_qpdf_init.cpp.obj")

    # Verify platform plugin exists
    if(NOT EXISTS ${QT_PLATFORM_PLUGIN_LIB})
        message(WARNING "Qt platform plugin not found at: ${QT_PLATFORM_PLUGIN_LIB}")
        # Try debug version
        set(QT_PLATFORM_PLUGIN_LIB "${THIRDPARTY_DIR}/qtbase_x64-windows-static/debug/Qt6/plugins/platforms/qwindowsd.lib")
        if(EXISTS ${QT_PLATFORM_PLUGIN_LIB})
            message(STATUS "Using debug Qt platform plugin: ${QT_PLATFORM_PLUGIN_LIB}")
        endif()
    else()
        message(STATUS "Found Qt platform plugin: ${QT_PLATFORM_PLUGIN_LIB}")
    endif()

    # Link Qt libraries - Force static linking approach
    target_link_libraries(lupine-editor LupineEngine
        # Qt static libraries
        ${QT6_CORE_LIB} ${QT6_WIDGETS_LIB} ${QT6_GUI_LIB} ${QT6_OPENGL_LIB} ${QT6_OPENGL_WIDGETS_LIB}
        # Qt EntryPoint for static linking
        ${QT6_ENTRYPOINT_LIB}
        # Qt platform plugins for static linking
        ${QT_PLATFORM_PLUGIN_LIB}
        # Qt resource object files for static linking
        ${QT_GUI_SHADERS_OBJ} ${QT_PDF_INIT_OBJ}
        # Qt dependency libraries (including freetype for full static linking)
        ${HARFBUZZ_LIB} ${ICU_UC_LIB} ${ICU_IN_LIB} ${ICU_DT_LIB} ${PCRE2_16_LIB} ${ZSTD_LIB}
        ${FREETYPE_LIB} ${LIBPNG_LIB} ${ZLIB_LIB} ${BROTLI_DEC_LIB} ${BROTLI_COMMON_LIB} ${DOUBLE_CONVERSION_LIB}
        # Additional static libraries to prevent DLL dependencies
        ${BZIP2_LIB} ${LIBJPEG_TURBO_LIB} ${TURBOJPEG_LIB} ${LZMA_LIB} ${LZ4_LIB} ${WAVPACK_LIB}
        # Audio library
        ${SNDFILE_LIB}
        # Windows system libraries required for Qt static linking
        user32 gdi32 shell32 ole32 oleaut32 uuid comdlg32 advapi32 winmm ws2_32 kernel32
        imm32 wininet rpcrt4 winspool setupapi version dwmapi uxtheme
        # OpenGL libraries
        opengl32 glu32
        # Additional Qt static dependencies
        synchronization netapi32 userenv authz dwrite d3d11 dxgi d3d12 dxguid
        # Additional Windows libraries for static linking
        crypt32 secur32 bcrypt wtsapi32 legacy_stdio_definitions
        # Windows Runtime and UIA libraries for Qt platform plugin
        windowsapp shcore d3d9 uiautomationcore
        # Additional system libraries for full static linking
        mpr propsys shlwapi iphlpapi psapi powrprof
        # Additional libraries for SSL/crypto and media support
        wldap32 normaliz
    )
    message(STATUS "Using static Qt6 library linking with all dependencies")

    # Apply same linker options to editor executable
    if(WIN32)
        target_link_options(lupine-editor PRIVATE
            $<$<CONFIG:Debug>:/NODEFAULTLIB:MSVCRT>
            $<$<CONFIG:Release>:/NODEFAULTLIB:MSVCRTD>
            # Force static linking and exclude dynamic runtime libraries
            /NODEFAULTLIB:msvcrt.dll
            /NODEFAULTLIB:msvcrtd.dll
            /NODEFAULTLIB:VCRUNTIME140.dll
            /NODEFAULTLIB:VCRUNTIME140_1.dll
            # Force static linking of all libraries
            /NODEFAULTLIB:SDL2.dll
            /NODEFAULTLIB:freetype.dll
            /NODEFAULTLIB:zlib1.dll
            /NODEFAULTLIB:brotlidec.dll
            /NODEFAULTLIB:brotlicommon.dll
            /NODEFAULTLIB:libpng16.dll
            /NODEFAULTLIB:bz2.dll
        )
    endif()

    # Set target properties for Qt
    set_target_properties(lupine-editor PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
    )

    message(STATUS "Qt6 found - lupine-editor will be built")
else()
    message(STATUS "Qt6 not found - lupine-editor will not be built")
endif()

# Web export template support
option(BUILD_WEB_TEMPLATE "Build web export template" OFF)

if(BUILD_WEB_TEMPLATE AND EMSCRIPTEN)
    message(STATUS "Building web export template")

    # Web template executable
    add_executable(lupine-web-template
        src/export/web/WebRuntimeMain.cpp
        src/export/web/WebScriptBridge.cpp
        ${ENGINE_SOURCES}
    )

    # Link libraries for web template
    target_link_libraries(lupine-web-template
        ${CMAKE_DL_LIBS}
    )

    # Web-specific compile flags
    target_compile_definitions(lupine-web-template PRIVATE
        LUPINE_WEB_BUILD=1
        LUPINE_PLATFORM_WEB=1
    )

    # Emscripten-specific settings
    set_target_properties(lupine-web-template PROPERTIES
        LINK_FLAGS "-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -s EXPORTED_FUNCTIONS=['_main','_lupine_main'] --preload-file assets@/assets"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web"
    )

    message(STATUS "Web template will be built as lupine-web-template")
endif()

# Set output directories
set_target_properties(LupineEngine lupine-cli lupine-runtime
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Set output directories for editor if it exists
if(TARGET lupine-editor)
    set_target_properties(lupine-editor
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# Copy third-party DLLs to output directory (Windows) - DISABLED for full static linking
# Note: With full static linking, no DLLs should be needed.
if(WIN32 AND FALSE)  # Disabled for full static linking
    # Add custom command to copy DLLs after build
    add_custom_command(TARGET lupine-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${THIRDPARTY_DIR}/sdl2_x64-windows/bin/SDL2.dll"
        "${CMAKE_BINARY_DIR}/bin/"
    )

    add_custom_command(TARGET lupine-runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${THIRDPARTY_DIR}/sdl2_x64-windows/bin/SDL2.dll"
        "${CMAKE_BINARY_DIR}/bin/"
    )

    # Copy DLLs for editor if it exists
    if(TARGET lupine-editor)
        add_custom_command(TARGET lupine-editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${THIRDPARTY_DIR}/sdl2_x64-windows/bin/SDL2.dll"
            "${CMAKE_BINARY_DIR}/bin/"
        )

        # Copy Qt DLLs for editor
        add_custom_command(TARGET lupine-editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${THIRDPARTY_DIR}/qtbase_x64-windows/bin/Qt6Core.dll"
            "${CMAKE_BINARY_DIR}/bin/"
        )
        add_custom_command(TARGET lupine-editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${THIRDPARTY_DIR}/qtbase_x64-windows/bin/Qt6Widgets.dll"
            "${CMAKE_BINARY_DIR}/bin/"
        )
        add_custom_command(TARGET lupine-editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${THIRDPARTY_DIR}/qtbase_x64-windows/bin/Qt6Gui.dll"
            "${CMAKE_BINARY_DIR}/bin/"
        )
        add_custom_command(TARGET lupine-editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${THIRDPARTY_DIR}/qtbase_x64-windows/bin/Qt6OpenGL.dll"
            "${CMAKE_BINARY_DIR}/bin/"
        )
        add_custom_command(TARGET lupine-editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${THIRDPARTY_DIR}/qtbase_x64-windows/bin/Qt6OpenGLWidgets.dll"
            "${CMAKE_BINARY_DIR}/bin/"
        )
    endif()
endif()

# Test executables removed for release builds
