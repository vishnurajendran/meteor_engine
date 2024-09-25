#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "sfml-system" for configuration "Release"
set_property(TARGET sfml-system APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(sfml-system PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libsfml-system.2.5.1.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libsfml-system.2.5.dylib"
  )

list(APPEND _cmake_import_check_targets sfml-system )
list(APPEND _cmake_import_check_files_for_sfml-system "${_IMPORT_PREFIX}/lib/libsfml-system.2.5.1.dylib" )

# Import target "sfml-window" for configuration "Release"
set_property(TARGET sfml-window APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(sfml-window PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libsfml-window.2.5.1.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libsfml-window.2.5.dylib"
  )

list(APPEND _cmake_import_check_targets sfml-window )
list(APPEND _cmake_import_check_files_for_sfml-window "${_IMPORT_PREFIX}/lib/libsfml-window.2.5.1.dylib" )

# Import target "sfml-graphics" for configuration "Release"
set_property(TARGET sfml-graphics APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(sfml-graphics PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libsfml-graphics.2.5.1.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libsfml-graphics.2.5.dylib"
  )

list(APPEND _cmake_import_check_targets sfml-graphics )
list(APPEND _cmake_import_check_files_for_sfml-graphics "${_IMPORT_PREFIX}/lib/libsfml-graphics.2.5.1.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
