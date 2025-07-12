# FindLupineDependencies.cmake
# Helper module to find all Lupine Engine dependencies across platforms

include(FindPackageHandleStandardArgs)

# Function to find a library with multiple possible names
function(lupine_find_library VAR_NAME)
    set(options REQUIRED)
    set(oneValueArgs)
    set(multiValueArgs NAMES PATHS)
    cmake_parse_arguments(LUPINE_FIND "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    find_library(${VAR_NAME}
        NAMES ${LUPINE_FIND_NAMES}
        PATHS ${LUPINE_FIND_PATHS}
        PATH_SUFFIXES lib lib64
    )
    
    if(LUPINE_FIND_REQUIRED AND NOT ${VAR_NAME})
        message(FATAL_ERROR "Required library not found: ${LUPINE_FIND_NAMES}")
    endif()
endfunction()

# Function to find multiple libraries for a component
function(lupine_find_component COMPONENT_NAME)
    set(options REQUIRED)
    set(oneValueArgs)
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(LUPINE_COMP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    set(${COMPONENT_NAME}_FOUND TRUE)
    set(${COMPONENT_NAME}_LIBRARIES)
    
    foreach(lib_spec IN LISTS LUPINE_COMP_LIBRARIES)
        string(REPLACE ":" ";" lib_parts "${lib_spec}")
        list(GET lib_parts 0 var_name)
        list(GET lib_parts 1 lib_names)
        string(REPLACE "," ";" lib_name_list "${lib_names}")
        
        lupine_find_library(${var_name} NAMES ${lib_name_list})
        
        if(${var_name})
            list(APPEND ${COMPONENT_NAME}_LIBRARIES ${${var_name}})
        else()
            set(${COMPONENT_NAME}_FOUND FALSE)
            if(LUPINE_COMP_REQUIRED)
                message(FATAL_ERROR "${COMPONENT_NAME}: Required library ${var_name} not found")
            endif()
        endif()
    endforeach()
    
    # Export to parent scope
    set(${COMPONENT_NAME}_FOUND ${${COMPONENT_NAME}_FOUND} PARENT_SCOPE)
    set(${COMPONENT_NAME}_LIBRARIES ${${COMPONENT_NAME}_LIBRARIES} PARENT_SCOPE)
endfunction()

# SDL2 Component
lupine_find_component(SDL2
    LIBRARIES
        "SDL2_MAIN_LIB:SDL2main,SDL2Main"
        "SDL2_LIB:SDL2,SDL2-static"
        "SDL2_IMAGE_LIB:SDL2_image,SDL2_image-static"
        "SDL2_TTF_LIB:SDL2_ttf,SDL2_ttf-static"
        "SDL2_MIXER_LIB:SDL2_mixer,SDL2_mixer-static"
)

# Graphics Component
lupine_find_component(GRAPHICS
    LIBRARIES
        "GLAD_LIB:glad,glad-static"
)

# Find GLM (header-only library)
find_path(GLM_INCLUDE_DIR
    NAMES glm/glm.hpp
    PATHS
        /usr/include
        /usr/local/include
        /opt/homebrew/include
        ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES include
)

if(GLM_INCLUDE_DIR)
    message(STATUS "Found GLM: ${GLM_INCLUDE_DIR}")
    set(GLM_FOUND TRUE)
else()
    message(STATUS "GLM not found")
    set(GLM_FOUND FALSE)
endif()

# Physics Component
lupine_find_component(PHYSICS
    LIBRARIES
        "BULLET_DYNAMICS_LIB:BulletDynamics,BulletDynamics-static"
        "BULLET_COLLISION_LIB:BulletCollision,BulletCollision-static"
        "BULLET_LINEARMATH_LIB:LinearMath,BulletLinearMath,LinearMath-static"
        "BOX2D_LIB:box2d,Box2D,box2d-static"
)

# Audio Component
lupine_find_component(AUDIO
    LIBRARIES
        "VORBIS_LIB:vorbis,libvorbis,vorbis-static"
        "VORBISFILE_LIB:vorbisfile,libvorbisfile,vorbisfile-static"
        "OGG_LIB:ogg,libogg,ogg-static"
        "FLAC_LIB:FLAC,libFLAC,FLAC-static"
        "OPUS_LIB:opus,libopus,opus-static"
        "MPG123_LIB:mpg123,libmpg123,mpg123-static"
        "SNDFILE_LIB:sndfile,libsndfile,sndfile-static"
)

# Scripting Component
lupine_find_component(SCRIPTING
    LIBRARIES
        "LUA_LIB:lua,lua5.4,lua54,lua-static"
        "PYTHON_LIB:python3.12,python312,python3,python"
)

# Utility Component
lupine_find_component(UTILITY
    LIBRARIES
        "YAML_CPP_LIB:yaml-cpp,yaml-cpp-static"
        "SPDLOG_LIB:spdlog,spdlog-static"
        "PUGIXML_LIB:pugixml,pugixml-static"
        "SQLITE3_LIB:sqlite3,sqlite3-static"
)

# Image Component
lupine_find_component(IMAGE
    LIBRARIES
        "PNG_LIB:png,libpng,png16,libpng16,png-static"
        "JPEG_LIB:jpeg,libjpeg,turbojpeg,jpeg-static"
        "FREETYPE_LIB:freetype,freetype-static"
)

# Compression Component
lupine_find_component(COMPRESSION
    LIBRARIES
        "ZLIB_LIB:z,zlib,zlib-static"
        "BZIP2_LIB:bz2,libbz2,bzip2,bzip2-static"
        "LZMA_LIB:lzma,liblzma,lzma-static"
        "LZ4_LIB:lz4,liblz4,lz4-static"
        "ZSTD_LIB:zstd,libzstd,zstd-static"
)

# 3D Model Component
lupine_find_component(MODEL
    LIBRARIES
        "ASSIMP_LIB:assimp,assimp-static"
)

# Crypto Component
lupine_find_component(CRYPTO
    LIBRARIES
        "SSL_LIB:ssl,libssl,ssl-static"
        "CRYPTO_LIB:crypto,libcrypto,crypto-static"
)

# Collect all found libraries
set(LUPINE_ALL_LIBRARIES)

# Add libraries from each component
foreach(component IN ITEMS SDL2 GRAPHICS PHYSICS AUDIO SCRIPTING UTILITY IMAGE COMPRESSION MODEL CRYPTO)
    if(${component}_FOUND)
        list(APPEND LUPINE_ALL_LIBRARIES ${${component}_LIBRARIES})
        message(STATUS "Found ${component} component")
    else()
        message(STATUS "Missing ${component} component")
    endif()
endforeach()

# Remove duplicates and empty entries
list(REMOVE_DUPLICATES LUPINE_ALL_LIBRARIES)
list(REMOVE_ITEM LUPINE_ALL_LIBRARIES "")

# Platform-specific system libraries
if(WIN32)
    list(APPEND LUPINE_ALL_LIBRARIES
        user32 gdi32 shell32 advapi32 kernel32 ws2_32 opengl32
        winmm imm32 ole32 oleaut32 version setupapi
    )
elseif(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(OPENGL_FRAMEWORK OpenGL)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    
    list(APPEND LUPINE_ALL_LIBRARIES
        ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK} ${OPENGL_FRAMEWORK}
        ${COREAUDIO_FRAMEWORK} ${AUDIOTOOLBOX_FRAMEWORK}
    )
elseif(UNIX)
    find_package(Threads REQUIRED)
    find_package(X11)
    
    list(APPEND LUPINE_ALL_LIBRARIES
        Threads::Threads ${CMAKE_DL_LIBS}
    )
    
    if(X11_FOUND)
        list(APPEND LUPINE_ALL_LIBRARIES ${X11_LIBRARIES})
    endif()
    
    # Audio libraries
    find_library(ASOUND_LIB asound)
    if(ASOUND_LIB)
        list(APPEND LUPINE_ALL_LIBRARIES ${ASOUND_LIB})
    endif()
endif()

# Export results
set(LupineDependencies_FOUND TRUE)
set(LupineDependencies_LIBRARIES ${LUPINE_ALL_LIBRARIES})

message(STATUS "LupineDependencies: Found ${CMAKE_CURRENT_LIST_DIR}")
list(LENGTH LUPINE_ALL_LIBRARIES lib_count)
message(STATUS "LupineDependencies: Collected ${lib_count} libraries")

# Handle standard find_package arguments
find_package_handle_standard_args(LupineDependencies
    FOUND_VAR LupineDependencies_FOUND
    REQUIRED_VARS LUPINE_ALL_LIBRARIES
)
