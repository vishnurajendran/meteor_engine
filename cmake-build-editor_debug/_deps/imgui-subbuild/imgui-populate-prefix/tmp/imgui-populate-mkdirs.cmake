# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-src"
  "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-build"
  "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-subbuild/imgui-populate-prefix"
  "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-subbuild/imgui-populate-prefix/tmp"
  "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp"
  "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-subbuild/imgui-populate-prefix/src"
  "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/vishnurajendran/Documents/GitHub/meteor_engine/cmake-build-editor_debug/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
