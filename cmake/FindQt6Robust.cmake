# FindQt6Robust.cmake
# Robust Qt6 finder that handles static builds and avoids hanging

include(FindPackageHandleStandardArgs)

# Function to find Qt6 with timeout protection
function(find_qt6_robust)
    set(options REQUIRED)
    set(oneValueArgs)
    set(multiValueArgs COMPONENTS)
    cmake_parse_arguments(QT6_ROBUST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Set variables to prevent hanging on static builds
    set(QT_NO_CREATE_VERSIONLESS_TARGETS ON PARENT_SCOPE)
    set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON PARENT_SCOPE)
    
    # Disable problematic system dependencies that can cause hangs
    set(CMAKE_DISABLE_FIND_PACKAGE_WrapSystemDoubleConversion ON PARENT_SCOPE)
    set(CMAKE_DISABLE_FIND_PACKAGE_WrapSystemHarfbuzz ON PARENT_SCOPE)
    set(CMAKE_DISABLE_FIND_PACKAGE_WrapSystemFreetype ON PARENT_SCOPE)
    set(CMAKE_DISABLE_FIND_PACKAGE_WrapSystemPCRE2 ON PARENT_SCOPE)
    set(CMAKE_DISABLE_FIND_PACKAGE_WrapSystemZLIB ON PARENT_SCOPE)
    set(CMAKE_DISABLE_FIND_PACKAGE_WrapSystemPNG ON PARENT_SCOPE)
    
    # Force static linking for Qt
    if(WIN32)
        set(QT_FEATURE_static ON PARENT_SCOPE)
        set(QT_FEATURE_shared OFF PARENT_SCOPE)
        add_compile_definitions(QT_STATIC_BUILD)
    endif()
    
    # Try to find Qt6 components one by one to identify issues
    set(QT6_ALL_FOUND TRUE)
    set(QT6_FOUND_COMPONENTS)
    set(QT6_MISSING_COMPONENTS)
    
    foreach(component IN LISTS QT6_ROBUST_COMPONENTS)
        message(STATUS "Searching for Qt6${component}...")
        
        # Use QUIET to avoid verbose output that might indicate hanging
        find_package(Qt6${component} QUIET)
        
        if(Qt6${component}_FOUND)
            list(APPEND QT6_FOUND_COMPONENTS ${component})
            message(STATUS "  ✓ Qt6${component} found")
        else()
            list(APPEND QT6_MISSING_COMPONENTS ${component})
            message(STATUS "  ✗ Qt6${component} not found")
            set(QT6_ALL_FOUND FALSE)
        endif()
    endforeach()
    
    # Export results to parent scope
    set(Qt6_FOUND ${QT6_ALL_FOUND} PARENT_SCOPE)
    set(Qt6_FOUND_COMPONENTS ${QT6_FOUND_COMPONENTS} PARENT_SCOPE)
    set(Qt6_MISSING_COMPONENTS ${QT6_MISSING_COMPONENTS} PARENT_SCOPE)
    
    if(QT6_ALL_FOUND)
        message(STATUS "All Qt6 components found: ${QT6_FOUND_COMPONENTS}")
    else()
        message(STATUS "Missing Qt6 components: ${QT6_MISSING_COMPONENTS}")
        if(QT6_ROBUST_REQUIRED)
            message(FATAL_ERROR "Required Qt6 components not found")
        endif()
    endif()
endfunction()

# Function to setup Qt6 tools
function(setup_qt6_tools)
    # Find Qt tools
    set(QT_TOOL_SEARCH_PATHS)
    
    # Add platform-specific tool paths
    if(WIN32)
        # Windows vcpkg paths
        if(DEFINED VCPKG_INSTALLED_DIR)
            list(APPEND QT_TOOL_SEARCH_PATHS
                "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin"
                "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
            )
        endif()
        
        # Windows Qt installation paths
        if(DEFINED QT_HOST_PATH)
            list(APPEND QT_TOOL_SEARCH_PATHS
                "${QT_HOST_PATH}/bin"
                "${QT_HOST_PATH}/Tools/Qt6/bin"
            )
        endif()
    elseif(APPLE)
        # macOS Homebrew paths
        list(APPEND QT_TOOL_SEARCH_PATHS
            "/opt/homebrew/opt/qt6/bin"
            "/usr/local/opt/qt6/bin"
            "/opt/homebrew/bin"
            "/usr/local/bin"
        )
    else()
        # Linux system paths
        list(APPEND QT_TOOL_SEARCH_PATHS
            "/usr/lib/qt6/bin"
            "/usr/lib/x86_64-linux-gnu/qt6/bin"
            "/usr/bin"
        )
    endif()
    
    # Find required Qt tools
    find_program(Qt6_MOC_EXECUTABLE 
        NAMES moc moc.exe moc-qt6
        HINTS ${QT_TOOL_SEARCH_PATHS}
        DOC "Qt6 Meta Object Compiler"
    )
    
    find_program(Qt6_UIC_EXECUTABLE 
        NAMES uic uic.exe uic-qt6
        HINTS ${QT_TOOL_SEARCH_PATHS}
        DOC "Qt6 User Interface Compiler"
    )
    
    find_program(Qt6_RCC_EXECUTABLE 
        NAMES rcc rcc.exe rcc-qt6
        HINTS ${QT_TOOL_SEARCH_PATHS}
        DOC "Qt6 Resource Compiler"
    )
    
    # Export to parent scope
    set(Qt6_MOC_EXECUTABLE ${Qt6_MOC_EXECUTABLE} PARENT_SCOPE)
    set(Qt6_UIC_EXECUTABLE ${Qt6_UIC_EXECUTABLE} PARENT_SCOPE)
    set(Qt6_RCC_EXECUTABLE ${Qt6_RCC_EXECUTABLE} PARENT_SCOPE)
    
    # Verify tools
    set(QT6_TOOLS_FOUND TRUE)
    if(NOT Qt6_MOC_EXECUTABLE)
        message(WARNING "Qt6 moc not found")
        set(QT6_TOOLS_FOUND FALSE)
    endif()
    if(NOT Qt6_UIC_EXECUTABLE)
        message(WARNING "Qt6 uic not found")
        set(QT6_TOOLS_FOUND FALSE)
    endif()
    if(NOT Qt6_RCC_EXECUTABLE)
        message(WARNING "Qt6 rcc not found")
        set(QT6_TOOLS_FOUND FALSE)
    endif()
    
    set(Qt6_TOOLS_FOUND ${QT6_TOOLS_FOUND} PARENT_SCOPE)
    
    if(QT6_TOOLS_FOUND)
        message(STATUS "Qt6 tools found:")
        message(STATUS "  MOC: ${Qt6_MOC_EXECUTABLE}")
        message(STATUS "  UIC: ${Qt6_UIC_EXECUTABLE}")
        message(STATUS "  RCC: ${Qt6_RCC_EXECUTABLE}")
    else()
        message(STATUS "Some Qt6 tools missing. Searched paths:")
        foreach(path IN LISTS QT_TOOL_SEARCH_PATHS)
            message(STATUS "  ${path}")
        endforeach()
    endif()
endfunction()

# Function to configure Qt6 for static linking
function(configure_qt6_static_linking TARGET_NAME)
    if(WIN32 AND TARGET ${TARGET_NAME})
        # Add static linking definitions
        target_compile_definitions(${TARGET_NAME} PRIVATE
            QT_STATIC_BUILD
            QT_NO_DYNAMIC_CAST
        )
        
        # Add Windows-specific Qt static libraries
        target_link_libraries(${TARGET_NAME} PRIVATE
            # Windows system libraries required for static Qt
            dwmapi uxtheme winmm imm32 wtsapi32 setupapi version
            netapi32 userenv crypt32 secur32 bcrypt
            # Additional libraries for Qt static build
            ws2_32 ole32 oleaut32 uuid shell32 advapi32
        )
        
        # Set entry point for static Qt builds
        if(TARGET Qt6::EntryPoint)
            target_link_libraries(${TARGET_NAME} PRIVATE Qt6::EntryPoint)
        endif()
    endif()
endfunction()

# Main function to find and configure Qt6
function(find_and_configure_qt6)
    set(options REQUIRED)
    set(oneValueArgs)
    set(multiValueArgs COMPONENTS)
    cmake_parse_arguments(QT6_CONFIG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    message(STATUS "=== Qt6 Robust Configuration ===")
    
    # Setup Qt tools first
    setup_qt6_tools()
    
    # Find Qt6 components
    if(QT6_CONFIG_REQUIRED)
        find_qt6_robust(REQUIRED COMPONENTS ${QT6_CONFIG_COMPONENTS})
    else()
        find_qt6_robust(COMPONENTS ${QT6_CONFIG_COMPONENTS})
    endif()
    
    # Export results
    set(Qt6_FOUND ${Qt6_FOUND} PARENT_SCOPE)
    set(Qt6_TOOLS_FOUND ${Qt6_TOOLS_FOUND} PARENT_SCOPE)
    set(Qt6_FOUND_COMPONENTS ${Qt6_FOUND_COMPONENTS} PARENT_SCOPE)
    set(Qt6_MISSING_COMPONENTS ${Qt6_MISSING_COMPONENTS} PARENT_SCOPE)
    
    if(Qt6_FOUND AND Qt6_TOOLS_FOUND)
        message(STATUS "Qt6 configuration successful")
    else()
        message(STATUS "Qt6 configuration failed or incomplete")
    endif()
endfunction()

# Handle standard find_package arguments
find_package_handle_standard_args(Qt6Robust
    FOUND_VAR Qt6Robust_FOUND
    REQUIRED_VARS Qt6_FOUND
)
