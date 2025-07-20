# Lupine Core Library
# This library contains all the core engine functionality

# Collect all source files using GLOB_RECURSE
file(GLOB_RECURSE LUPINE_CORE_SOURCES
    "src/*.cpp"
    "src/*.c"
)

file(GLOB_RECURSE LUPINE_CORE_HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Create the core library
add_library(lupine-core STATIC
    ${LUPINE_CORE_SOURCES}
    ${LUPINE_CORE_HEADERS}
)

# Set target properties
set_target_properties(lupine-core PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

# Include directories
target_include_directories(lupine-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(lupine-core
    PUBLIC
        # Core graphics and windowing
        $<IF:$<TARGET_EXISTS:SDL2::SDL2-static>,SDL2::SDL2-static,SDL2::SDL2>
        $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image-static>,SDL2_image::SDL2_image-static,SDL2_image::SDL2_image>
        $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf-static>,SDL2_ttf::SDL2_ttf-static,SDL2_ttf::SDL2_ttf>
        $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer-static>,SDL2_mixer::SDL2_mixer-static,SDL2_mixer::SDL2_mixer>
        $<TARGET_NAME_IF_EXISTS:glad::glad>
        $<TARGET_NAME_IF_EXISTS:glm::glm>

        # Physics engines
        $<TARGET_NAME_IF_EXISTS:box2d::box2d>
        $<TARGET_NAME_IF_EXISTS:BulletDynamics>
        $<TARGET_NAME_IF_EXISTS:BulletCollision>
        $<TARGET_NAME_IF_EXISTS:LinearMath>

        # 3D model loading
        $<TARGET_NAME_IF_EXISTS:assimp::assimp>

        # Scripting
        $<TARGET_NAME_IF_EXISTS:lua>
        $<$<BOOL:${LUPINE_ENABLE_PYTHON}>:pybind11::embed>

        # Data formats
        $<TARGET_NAME_IF_EXISTS:yaml-cpp::yaml-cpp>
        $<TARGET_NAME_IF_EXISTS:pugixml::pugixml>
        $<TARGET_NAME_IF_EXISTS:nlohmann_json::nlohmann_json>

        # Logging
        $<TARGET_NAME_IF_EXISTS:spdlog::spdlog>
        $<TARGET_NAME_IF_EXISTS:fmt::fmt>

        # Audio libraries
        $<TARGET_NAME_IF_EXISTS:SndFile::sndfile>
        $<TARGET_NAME_IF_EXISTS:Ogg::ogg>
        $<TARGET_NAME_IF_EXISTS:Vorbis::vorbis>
        $<TARGET_NAME_IF_EXISTS:Vorbis::vorbisfile>
        $<TARGET_NAME_IF_EXISTS:FLAC::FLAC>
        $<TARGET_NAME_IF_EXISTS:Opus::opus>

        # Image libraries
        $<TARGET_NAME_IF_EXISTS:PNG::PNG>
        $<TARGET_NAME_IF_EXISTS:JPEG::JPEG>
        $<TARGET_NAME_IF_EXISTS:Freetype::Freetype>

        # Compression libraries
        $<TARGET_NAME_IF_EXISTS:ZLIB::ZLIB>
        $<TARGET_NAME_IF_EXISTS:BZip2::BZip2>
        $<TARGET_NAME_IF_EXISTS:lz4::lz4>
        $<TARGET_NAME_IF_EXISTS:zstd::libzstd>
)

# Platform-specific libraries
if(LUPINE_PLATFORM_WINDOWS)
    # Tell SDL2 not to hijack main function
    target_compile_definitions(lupine-core PUBLIC SDL_MAIN_HANDLED)

    target_link_libraries(lupine-core
        PUBLIC
            opengl32
            gdi32
            winmm
            ole32
            oleaut32
            imm32
            version
    )
elseif(LUPINE_PLATFORM_LINUX)
    target_link_libraries(lupine-core
        PUBLIC
            GL
            X11
            Xrandr
            Xinerama
            Xi
            Xcursor
            dl
            pthread
    )
elseif(LUPINE_PLATFORM_MACOS)
    target_link_libraries(lupine-core
        PUBLIC
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
            "-framework CoreFoundation"
    )
endif()

# Additional include directories are handled by the target dependencies

# Compiler-specific settings
if(MSVC)
    target_compile_options(lupine-core PRIVATE /W4)
    target_compile_definitions(lupine-core PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(lupine-core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Export symbols for Windows DLL
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(lupine-core
        PRIVATE LUPINE_CORE_EXPORTS
        INTERFACE LUPINE_CORE_IMPORTS
    )
endif()

# Python support definition
if(LUPINE_ENABLE_PYTHON AND Python3_FOUND)
    target_compile_definitions(lupine-core PUBLIC LUPINE_PYTHON_SUPPORT)
endif()

# Graphics backend definitions
# Enable OpenGL backend for desktop platforms
if(NOT EMSCRIPTEN)
    target_compile_definitions(lupine-core PUBLIC LUPINE_OPENGL_BACKEND)
endif()

# Enable WebGL backend for web platforms
if(EMSCRIPTEN)
    target_compile_definitions(lupine-core PUBLIC LUPINE_WEBGL_BACKEND)
endif()

# Install targets
install(TARGETS lupine-core
    EXPORT LupineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Create alias for consistent naming
add_library(Lupine::Core ALIAS lupine-core)
