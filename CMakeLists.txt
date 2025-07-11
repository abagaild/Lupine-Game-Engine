cmake_minimum_required(VERSION 3.20)
project(LupineEngine VERSION 1.0.0 LANGUAGES CXX)

#--- C++ Standard & Build Type ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

#--- Compiler Flags ---
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_compile_options(/Zc:__cplusplus /permissive- /W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

#--- Platform Detection ---
if(WIN32)
  set(PLATFORM_NAME "Windows")
elseif(APPLE)
  if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
    set(PLATFORM_NAME "Mac-ARM64")
  else()
    set(PLATFORM_NAME "Mac-OSX")
  endif()
elseif(UNIX)
  set(PLATFORM_NAME "Linux")
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

#--- Third-Party Directories ---
set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
set(EMSDK_DIR       "${THIRDPARTY_DIR}/emsdk")
set(QT_DIR          "${THIRDPARTY_DIR}/Qt")
set(PLATFORM_DIR    "${THIRDPARTY_DIR}/${PLATFORM_NAME}")

# Platform-specific Qt paths
if(WIN32)
  set(QT_STATIC_DIR "${PLATFORM_DIR}/qtbase_x64-windows-static")
  set(Qt6_DIR "${QT_STATIC_DIR}/share/Qt6")
  # Keep QT_HOST_PATH pointing to main Qt installation for tools
  set(QT_HOST_PATH "${QT_DIR}")
  # Set host cmake directory for cross-compilation
  set(QT_HOST_PATH_CMAKE_DIR "${QT_DIR}/share" CACHE PATH "Host Qt CMake directory")
else()
  # For Mac/Linux, use the main Qt installation
  set(Qt6_DIR "${QT_DIR}/lib/cmake/Qt6")
  set(QT_HOST_PATH "${QT_DIR}")
endif()

# Add platform-specific directories to CMAKE_PREFIX_PATH
list(APPEND CMAKE_PREFIX_PATH
  ${EMSDK_DIR}
  ${QT_DIR}
  ${PLATFORM_DIR}
  ${QT_STATIC_DIR}
)

# Add all platform-specific package directories to CMAKE_PREFIX_PATH for find_package
if(EXISTS "${PLATFORM_DIR}")
  file(GLOB PLATFORM_PACKAGES "${PLATFORM_DIR}/*")
  foreach(package IN LISTS PLATFORM_PACKAGES)
    if(IS_DIRECTORY "${package}")
      list(APPEND CMAKE_PREFIX_PATH "${package}")
    endif()
  endforeach()
endif()

#--- Emscripten Support ---
option(BUILD_WEB_TEMPLATE "Build web export template" OFF)
if(BUILD_WEB_TEMPLATE)
  find_program(EMCC_EXECUTABLE emcc HINTS "${EMSDK_DIR}/bin" REQUIRED)
  set(CMAKE_C_COMPILER   ${EMCC_EXECUTABLE})
  set(CMAKE_CXX_COMPILER ${EMCC_EXECUTABLE})
  message(STATUS "Using Emscripten: ${EMCC_EXECUTABLE}")
endif()

#--- Qt Host Path for Cross-Compilation ---
# QT_HOST_PATH is already set above based on platform

# Debug: Print paths being used
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "QT_DIR: ${QT_DIR}")
message(STATUS "QT_HOST_PATH: ${QT_HOST_PATH}")
message(STATUS "PLATFORM_DIR: ${PLATFORM_DIR}")
if(QT_STATIC_DIR)
  message(STATUS "QT_STATIC_DIR: ${QT_STATIC_DIR}")
endif()

# Find Qt tools - prioritize main Qt installation for tools
find_program(Qt6_MOC_EXECUTABLE NAMES moc moc.exe HINTS
  "${QT_HOST_PATH}/Tools/Qt6/bin"
  "${QT_HOST_PATH}/tools/Qt6/bin"
  "${QT_HOST_PATH}/bin"
  "${QT_STATIC_DIR}/tools/Qt6/bin"
  NO_DEFAULT_PATH)

if(NOT Qt6_MOC_EXECUTABLE)
  message(STATUS "Searching for moc in system PATH...")
  find_program(Qt6_MOC_EXECUTABLE NAMES moc moc.exe)
endif()

find_program(Qt6_UIC_EXECUTABLE NAMES uic uic.exe HINTS
  "${QT_HOST_PATH}/Tools/Qt6/bin"
  "${QT_HOST_PATH}/tools/Qt6/bin"
  "${QT_HOST_PATH}/bin"
  "${QT_STATIC_DIR}/tools/Qt6/bin"
  NO_DEFAULT_PATH)

if(NOT Qt6_UIC_EXECUTABLE)
  message(STATUS "Searching for uic in system PATH...")
  find_program(Qt6_UIC_EXECUTABLE NAMES uic uic.exe)
endif()

find_program(Qt6_RCC_EXECUTABLE NAMES rcc rcc.exe HINTS
  "${QT_HOST_PATH}/Tools/Qt6/bin"
  "${QT_HOST_PATH}/tools/Qt6/bin"
  "${QT_HOST_PATH}/bin"
  "${QT_STATIC_DIR}/tools/Qt6/bin"
  NO_DEFAULT_PATH)

if(NOT Qt6_RCC_EXECUTABLE)
  message(STATUS "Searching for rcc in system PATH...")
  find_program(Qt6_RCC_EXECUTABLE NAMES rcc rcc.exe)
endif()

# Verify tools were found
if(NOT Qt6_MOC_EXECUTABLE OR NOT Qt6_UIC_EXECUTABLE OR NOT Qt6_RCC_EXECUTABLE)
  message(STATUS "Qt tools search paths:")
  message(STATUS "  ${QT_HOST_PATH}/Tools/Qt6/bin")
  message(STATUS "  ${QT_HOST_PATH}/tools/Qt6/bin")
  message(STATUS "  ${QT_HOST_PATH}/bin")
  if(QT_STATIC_DIR)
    message(STATUS "  ${QT_STATIC_DIR}/tools/Qt6/bin")
  endif()
  message(FATAL_ERROR "Required Qt tools not found. Please check Qt installation.")
endif()

message(STATUS "Found Qt tools:")
message(STATUS "  MOC: ${Qt6_MOC_EXECUTABLE}")
message(STATUS "  UIC: ${Qt6_UIC_EXECUTABLE}")
message(STATUS "  RCC: ${Qt6_RCC_EXECUTABLE}")

#--- Find Qt6 ---
message(STATUS "Using Qt6_DIR: ${Qt6_DIR}")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL OpenGLWidgets)

#--- Platform-specific Include Directories ---
set(PLATFORM_INCLUDES)

# Add platform-specific include directories
if(EXISTS "${PLATFORM_DIR}")
  file(GLOB PLATFORM_PACKAGES "${PLATFORM_DIR}/*")
  foreach(package IN LISTS PLATFORM_PACKAGES)
    if(IS_DIRECTORY "${package}")
      if(EXISTS "${package}/include")
        list(APPEND PLATFORM_INCLUDES "${package}/include")
      endif()
    endif()
  endforeach()
endif()

# Add specific library includes from platform-specific directory
if(WIN32)
  # Add GLAD include
  if(EXISTS "${PLATFORM_DIR}/glad_x64-windows-static/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/glad_x64-windows-static/include")
  endif()

  # Add SDL2 includes
  if(EXISTS "${PLATFORM_DIR}/sdl2_x64-windows-static/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/sdl2_x64-windows-static/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/sdl2_x64-windows-static/include/SDL2")
  endif()
  if(EXISTS "${PLATFORM_DIR}/sdl2-ttf_x64-windows-static/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/sdl2-ttf_x64-windows-static/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/sdl2-ttf_x64-windows-static/include/SDL2")
  endif()

  # Add Python includes
  if(EXISTS "${PLATFORM_DIR}/python3_x64-windows-static/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/python3_x64-windows-static/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/python3_x64-windows-static/include/python3.12")
  endif()
  if(EXISTS "${PLATFORM_DIR}/python3_x64-windows/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/python3_x64-windows/include")
    # Check for versioned Python include directory
    file(GLOB PYTHON_VERSION_DIRS "${PLATFORM_DIR}/python3_x64-windows/include/python*")
    foreach(python_dir IN LISTS PYTHON_VERSION_DIRS)
      if(IS_DIRECTORY "${python_dir}")
        list(APPEND PLATFORM_INCLUDES "${python_dir}")
      endif()
    endforeach()
  endif()

  # Add pybind11 includes
  if(EXISTS "${PLATFORM_DIR}/pybind11_x64-windows/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/pybind11_x64-windows/include")
  endif()
  if(EXISTS "${PLATFORM_DIR}/pybind11")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/pybind11")
  endif()

elseif(APPLE)
  if(EXISTS "${PLATFORM_DIR}/glad_x64-osx/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/glad_x64-osx/include")
  endif()
elseif(UNIX)
  if(EXISTS "${PLATFORM_DIR}/glad_x64-linux/include")
    list(APPEND PLATFORM_INCLUDES "${PLATFORM_DIR}/glad_x64-linux/include")
  endif()
endif()

#--- Include Directories ---
include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${EMSDK_DIR}/include"
  "${QT_DIR}/include"
  ${PLATFORM_INCLUDES}
)

#--- Engine Sources & Headers ---
file(GLOB_RECURSE ENGINE_SOURCES
  "src/animation/*.cpp"
  "src/audio/*.cpp"
  "src/components/*.cpp"
  "src/core/*.cpp"
  "src/export/*.cpp"
  "src/input/*.cpp"
  "src/localization/*.cpp"
  "src/nodes/*.cpp"
  "src/physics/*.cpp"
  "src/rendering/*.cpp"
  "src/resources/*.cpp"
  "src/scriptableobjects/*.cpp"
  "src/scripting/*.cpp"
  "src/serialization/*.cpp"
  "src/terrain/*.cpp"
  "src/tilemap/*.cpp"
  "src/visualscripting/*.cpp"
)

# Add GLAD source - check if we need to compile it or if it's already a library
set(GLAD_SOURCE_ADDED FALSE)
if(WIN32 AND EXISTS "${PLATFORM_DIR}/glad_x64-windows-static/lib/glad.lib")
  # Use precompiled GLAD library on Windows
  set(GLAD_LIBRARY "${PLATFORM_DIR}/glad_x64-windows-static/lib/glad.lib")
  set(GLAD_SOURCE_ADDED TRUE)
elseif(EXISTS "${THIRDPARTY_DIR}/glad/src/glad.c")
  # Use source file if available
  list(APPEND ENGINE_SOURCES "${THIRDPARTY_DIR}/glad/src/glad.c")
  set(GLAD_SOURCE_ADDED TRUE)
endif()

if(NOT GLAD_SOURCE_ADDED)
  message(WARNING "GLAD source or library not found. OpenGL loading may not work.")
endif()

file(GLOB_RECURSE ENGINE_HEADERS
  "include/lupine/*.h"
  "src/*/*.h"
)

#--- Engine Library ---
add_library(LupineEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

#--- Gather Platform-specific Libraries ---
set(PLATFORM_LIBS)

# Collect all static libraries from platform-specific packages
if(EXISTS "${PLATFORM_DIR}")
  file(GLOB PLATFORM_PACKAGES "${PLATFORM_DIR}/*")
  foreach(package IN LISTS PLATFORM_PACKAGES)
    if(IS_DIRECTORY "${package}")
      # Check for lib directory
      if(EXISTS "${package}/lib")
        if(WIN32)
          file(GLOB package_libs "${package}/lib/*.lib")
        else()
          file(GLOB package_libs "${package}/lib/*.a")
        endif()
        list(APPEND PLATFORM_LIBS ${package_libs})
      endif()

      # Check for debug lib directory
      if(EXISTS "${package}/debug/lib" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(WIN32)
          file(GLOB package_debug_libs "${package}/debug/lib/*.lib")
        else()
          file(GLOB package_debug_libs "${package}/debug/lib/*.a")
        endif()
        list(APPEND PLATFORM_LIBS ${package_debug_libs})
      endif()
    endif()
  endforeach()
endif()

# Add GLAD library if using precompiled version
if(DEFINED GLAD_LIBRARY)
  list(APPEND PLATFORM_LIBS ${GLAD_LIBRARY})
endif()

if(PLATFORM_LIBS)
  list(LENGTH PLATFORM_LIBS lib_count)
  message(STATUS "Found ${lib_count} platform libraries for ${PLATFORM_NAME}")
else()
  message(STATUS "No platform libraries found for ${PLATFORM_NAME}")
endif()

#--- Link Engine Dependencies ---
target_link_libraries(LupineEngine PRIVATE
  ${PLATFORM_LIBS}
  Qt6::Core Qt6::Widgets Qt6::Gui Qt6::OpenGL Qt6::OpenGLWidgets
)

# For static Qt builds on Windows, we need additional system libraries
if(WIN32 AND QT_STATIC_DIR)
  # Check if Qt6::EntryPoint is available
  if(TARGET Qt6::EntryPoint)
    target_link_libraries(LupineEngine PRIVATE Qt6::EntryPoint)
  endif()

  target_link_libraries(LupineEngine PRIVATE
    # Windows system libraries for static Qt
    dwmapi
    uxtheme
    winmm
    imm32
    wtsapi32
    setupapi
    version
    netapi32
    userenv
    # Additional libraries that might be needed
    crypt32
    secur32
    bcrypt
  )
endif()

#--- Platform System Libraries ---
if(WIN32)
  target_link_libraries(LupineEngine PRIVATE user32 gdi32 shell32 advapi32 kernel32 ws2_32 opengl32)
elseif(APPLE)
  find_library(COCOA Cocoa)
  find_library(IOKIT IOKit)
  target_link_libraries(LupineEngine PRIVATE ${COCOA} ${IOKIT} OpenGL)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(LupineEngine PRIVATE pthread dl X11 Threads::Threads)
endif()

#--- Platform-specific Compile Definitions ---
target_compile_definitions(LupineEngine PRIVATE
  LUPINE_EMBED_WINDOWS_LIBRARIES
  LUPINE_EMBED_LINUX_LIBRARIES
  LUPINE_EMBED_MACOS_LIBRARIES
  LUPINE_EMBED_ARM64_LIBRARIES
)

# Add platform-specific definitions
if(WIN32)
  target_compile_definitions(LupineEngine PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    # OpenGL extensions
    GL_GLEXT_PROTOTYPES
  )
  # For static Qt builds
  if(QT_STATIC_DIR)
    target_compile_definitions(LupineEngine PRIVATE QT_STATIC_BUILD)
  endif()
endif()

#--- Runtime Executable ---
add_executable(lupine-runtime "src/runtime/main.cpp")
if(WIN32)
  target_sources(lupine-runtime PRIVATE "src/runtime/resources/lupine-runtime.rc")
endif()

target_link_libraries(lupine-runtime PRIVATE LupineEngine)
set_target_properties(lupine-runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

#--- Qt Editor Executable ---
if(Qt6_FOUND)
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTORCC ON)
  file(GLOB_RECURSE EDITOR_SOURCES "src/editor/*.cpp")
  file(GLOB_RECURSE EDITOR_HEADERS "src/editor/*.h" "include/lupine/editor/*.h")
  set(EDITOR_RESOURCES "src/editor/resources/editor.qrc")
  if(WIN32)
    list(APPEND EDITOR_RESOURCES "src/editor/resources/lupine-editor.rc")
  endif()
  add_executable(lupine-editor ${EDITOR_SOURCES} ${EDITOR_HEADERS} ${EDITOR_RESOURCES})
  target_link_libraries(lupine-editor PRIVATE LupineEngine
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::OpenGL Qt6::OpenGLWidgets
  )

  # For static Qt builds on Windows, add additional libraries to editor
  if(WIN32 AND QT_STATIC_DIR)
    # Check if Qt6::EntryPoint is available
    if(TARGET Qt6::EntryPoint)
      target_link_libraries(lupine-editor PRIVATE Qt6::EntryPoint)
    endif()

    target_link_libraries(lupine-editor PRIVATE
      dwmapi
      uxtheme
      winmm
      imm32
      wtsapi32
      setupapi
      version
      netapi32
      userenv
      crypt32
      secur32
      bcrypt
    )
  endif()
  set_target_properties(lupine-editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
  )
endif()

#--- Web Export Template ---
if(BUILD_WEB_TEMPLATE)
  add_executable(lupine-web-template
    "src/export/web/WebRuntimeMain.cpp"
    "src/export/web/WebScriptBridge.cpp"
    ${ENGINE_SOURCES}
  )
  target_compile_definitions(lupine-web-template PRIVATE LUPINE_WEB_BUILD=1 LUPINE_PLATFORM_WEB=1)
  set_target_properties(lupine-web-template PROPERTIES
    LINK_FLAGS "-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web"
  )
endif()

#--- Output Directories ---
set(TARGETS_TO_SET_PROPERTIES LupineEngine lupine-runtime)
if(Qt6_FOUND)
  list(APPEND TARGETS_TO_SET_PROPERTIES lupine-editor)
endif()
if(BUILD_WEB_TEMPLATE)
  list(APPEND TARGETS_TO_SET_PROPERTIES lupine-web-template)
endif()

set_target_properties(
  ${TARGETS_TO_SET_PROPERTIES}
  PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
