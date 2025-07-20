# Lupine Runtime Executable
# This is the game runtime that loads and runs Lupine projects

# Collect all source files
file(GLOB_RECURSE LUPINE_RUNTIME_SOURCES
    "src/*.cpp"
    "src/*.c"
)

file(GLOB_RECURSE LUPINE_RUNTIME_HEADERS
    "src/*.h"
    "src/*.hpp"
)

# Create the runtime executable
add_executable(lupine-runtime
    ${LUPINE_RUNTIME_SOURCES}
    ${LUPINE_RUNTIME_HEADERS}
)

# Set target properties
set_target_properties(lupine-runtime PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "lupine-runtime"
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

# Include directories
target_include_directories(lupine-runtime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link with lupine-core
target_link_libraries(lupine-runtime
    PRIVATE
        lupine-core
)

# Platform-specific settings
if(LUPINE_PLATFORM_WINDOWS)
    # Windows-specific settings
    set_target_properties(lupine-runtime PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # Add Windows resource file if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/resources/lupine-runtime.rc")
        target_sources(lupine-runtime PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/resources/lupine-runtime.rc"
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
    
elseif(LUPINE_PLATFORM_LINUX)
    # Linux-specific settings
    set_target_properties(lupine-runtime PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    
elseif(LUPINE_PLATFORM_MACOS)
    # macOS-specific settings
    set_target_properties(lupine-runtime PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Lupine Runtime"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_IDENTIFIER "org.lupine-engine.runtime"
    )
    
    # Copy icon file to bundle if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/icon.png")
        set_target_properties(lupine-runtime PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "icon.png"
        )
        
        # Copy icon to Resources folder
        add_custom_command(TARGET lupine-runtime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/icon.png"
                "$<TARGET_BUNDLE_DIR:lupine-runtime>/Contents/Resources/icon.png"
        )
    endif()
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(lupine-runtime PRIVATE /W4)
    
    # Set subsystem to console for runtime
    set_target_properties(lupine-runtime PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
else()
    target_compile_options(lupine-runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install the runtime executable
install(TARGETS lupine-runtime
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install additional runtime files
if(LUPINE_PLATFORM_WINDOWS)
    # Install any required DLLs on Windows
    # This would be handled by vcpkg or manual DLL copying
endif()

# Create a desktop entry for Linux
if(LUPINE_PLATFORM_LINUX)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/lupine-runtime.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/lupine-runtime.desktop"
        @ONLY
    )
    
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lupine-runtime.desktop"
        DESTINATION share/applications
    )
endif()

# Copy assets and resources
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/lupine-runtime
    )
endif()

# Development convenience: copy runtime to root build directory
add_custom_command(TARGET lupine-runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:lupine-runtime>
        "${CMAKE_BINARY_DIR}/$<TARGET_FILE_NAME:lupine-runtime>"
    COMMENT "Copying runtime to build root"
)
