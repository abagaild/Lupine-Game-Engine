# Lupine Engine (Editor) Executable
# This is the Qt-based editor application

# Only build if Qt is available
if(NOT Qt6_FOUND)
    message(WARNING "Qt6 not found, skipping lupine-engine build")
    return()
endif()

# Collect all source files
file(GLOB_RECURSE LUPINE_ENGINE_SOURCES
    "src/*.cpp"
    "src/*.c"
)

file(GLOB_RECURSE LUPINE_ENGINE_HEADERS
    "src/*.h"
    "src/*.hpp"
    "include/*.h"
    "include/*.hpp"
)

# Find Qt UI files
file(GLOB_RECURSE LUPINE_ENGINE_UI_FILES
    "src/*.ui"
)

# Find Qt resource files
file(GLOB_RECURSE LUPINE_ENGINE_QRC_FILES
    "src/*.qrc"
)

# Create the engine executable
add_executable(lupine-engine
    ${LUPINE_ENGINE_SOURCES}
    ${LUPINE_ENGINE_HEADERS}
    ${LUPINE_ENGINE_UI_FILES}
    ${LUPINE_ENGINE_QRC_FILES}
)

# Set target properties
set_target_properties(lupine-engine PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "lupine-editor"
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# Include directories
target_include_directories(lupine-engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lupine-editor
)

# Link with lupine-core and Qt
target_link_libraries(lupine-engine
    PRIVATE
        lupine-core
        $<TARGET_NAME_IF_EXISTS:Qt6::Core>
        $<TARGET_NAME_IF_EXISTS:Qt6::Widgets>
        $<TARGET_NAME_IF_EXISTS:Qt6::OpenGL>
        $<TARGET_NAME_IF_EXISTS:Qt6::OpenGLWidgets>
        $<TARGET_NAME_IF_EXISTS:Qt6::Svg>
)

# Platform-specific settings
if(LUPINE_PLATFORM_WINDOWS)
    # Windows-specific settings
    set_target_properties(lupine-engine PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Tell SDL2 not to hijack main function for Qt applications
    target_compile_definitions(lupine-engine PRIVATE SDL_MAIN_HANDLED)
    
    # Add Windows resource file if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/resources/lupine-editor.rc")
        target_sources(lupine-engine PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/resources/lupine-editor.rc"
        )
    endif()
    
    # Copy icon file to build directory
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.ico")
        configure_file(
            "${CMAKE_SOURCE_DIR}/icon.ico"
            "${CMAKE_CURRENT_BINARY_DIR}/icon.ico"
            COPYONLY
        )
    endif()
    
    # Link Qt static plugins for Windows static builds
    if(Qt6_VERSION VERSION_GREATER_EQUAL "6.0.0")
        qt6_import_plugins(lupine-engine
            INCLUDE
                Qt6::QWindowsIntegrationPlugin
                Qt6::QMinimalIntegrationPlugin
                Qt6::QOffscreenIntegrationPlugin
                Qt6::QGifPlugin
                Qt6::QICOPlugin
                Qt6::QJpegPlugin
                Qt6::QTuioTouchPlugin
                Qt6::QModernWindowsStylePlugin
            EXCLUDE
                Qt6::QWindowsDirect2DIntegrationPlugin
        )
    endif()
    
elseif(LUPINE_PLATFORM_LINUX)
    # Linux-specific settings
    set_target_properties(lupine-engine PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    
elseif(LUPINE_PLATFORM_MACOS)
    # macOS-specific settings
    set_target_properties(lupine-engine PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Lupine Editor"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_IDENTIFIER "org.lupine-engine.editor"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.lupine-engine.editor"
    )
    
    # Copy icon file to bundle if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.png")
        set_target_properties(lupine-engine PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "icon.png"
        )
        
        # Copy icon to Resources folder
        add_custom_command(TARGET lupine-engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/icon.png"
                "$<TARGET_BUNDLE_DIR:lupine-engine>/Contents/Resources/icon.png"
        )
    endif()
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(lupine-engine PRIVATE /W4)
    
    # Set subsystem to windows for GUI application
    set_target_properties(lupine-engine PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
else()
    target_compile_options(lupine-engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install the engine executable
install(TARGETS lupine-engine
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create a desktop entry for Linux
if(LUPINE_PLATFORM_LINUX)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/lupine-editor.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/lupine-editor.desktop"
        @ONLY
    )
    
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lupine-editor.desktop"
        DESTINATION share/applications
    )
endif()

# Install Qt libraries and plugins (for deployment)
if(LUPINE_PLATFORM_WINDOWS AND Qt6_FOUND)
    # Use windeployqt for Windows deployment
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET lupine-engine POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:lupine-engine>
            COMMENT "Deploying Qt libraries"
        )
    endif()
endif()

# Development convenience: copy editor to root build directory
add_custom_command(TARGET lupine-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:lupine-engine>
        "${CMAKE_BINARY_DIR}/$<TARGET_FILE_NAME:lupine-engine>"
    COMMENT "Copying editor to build root"
)
