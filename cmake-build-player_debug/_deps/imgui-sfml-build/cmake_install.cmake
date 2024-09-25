# Install script for directory: /Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-sfml-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-sfml-build/libImGui-SFML_d.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libImGui-SFML_d.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libImGui-SFML_d.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/sfml-build/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libImGui-SFML_d.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libImGui-SFML_d.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-sfml-src/imgui-SFML.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-sfml-src/imgui-SFML_export.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-src/imconfig.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-src/imgui.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-src/imgui_internal.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-src/imstb_rectpack.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-src/imstb_textedit.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-src/imstb_truetype.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-src/misc/cpp/imgui_stdlib.h"
    "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-sfml-src/imconfig-SFML.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML/ImGui-SFMLConfig.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML/ImGui-SFMLConfig.cmake"
         "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-sfml-build/CMakeFiles/Export/761a6a4c7704629aea6d1d08969b2ac8/ImGui-SFMLConfig.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML/ImGui-SFMLConfig-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML/ImGui-SFMLConfig.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML" TYPE FILE FILES "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-sfml-build/CMakeFiles/Export/761a6a4c7704629aea6d1d08969b2ac8/ImGui-SFMLConfig.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML" TYPE FILE FILES "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-player_debug/_deps/imgui-sfml-build/CMakeFiles/Export/761a6a4c7704629aea6d1d08969b2ac8/ImGui-SFMLConfig-debug.cmake")
  endif()
endif()

