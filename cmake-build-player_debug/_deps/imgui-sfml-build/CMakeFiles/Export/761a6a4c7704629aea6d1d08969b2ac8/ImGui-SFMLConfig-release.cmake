#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ImGui-SFML::ImGui-SFML" for configuration "Release"
set_property(TARGET ImGui-SFML::ImGui-SFML APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(ImGui-SFML::ImGui-SFML PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libImGui-SFML.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libImGui-SFML.dylib"
  )

list(APPEND _cmake_import_check_targets ImGui-SFML::ImGui-SFML )
list(APPEND _cmake_import_check_files_for_ImGui-SFML::ImGui-SFML "${_IMPORT_PREFIX}/lib/libImGui-SFML.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
