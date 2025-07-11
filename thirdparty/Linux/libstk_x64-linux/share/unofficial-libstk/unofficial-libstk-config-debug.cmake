#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::libstk::libstk" for configuration "Debug"
set_property(TARGET unofficial::libstk::libstk APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::libstk::libstk PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/liblibstk.a"
  )

list(APPEND _cmake_import_check_targets unofficial::libstk::libstk )
list(APPEND _cmake_import_check_files_for_unofficial::libstk::libstk "${_IMPORT_PREFIX}/debug/lib/liblibstk.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
