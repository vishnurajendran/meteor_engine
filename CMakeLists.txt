cmake_minimum_required(VERSION 3.26)

# Set the directory where executables will be placed
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

project(meteor_engine)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/src)

# Define build options for Editor and Player modes
option(BUILD_EDITOR "Build the editor application" OFF)
option(BUILD_PLAYER "Build the player application" OFF)

# Add the executable and its source files
add_executable(meteor_engine
        src/main.cpp
        src/core/object/object.cpp
        src/core/object/object.h
        src/core/object/gc.cpp
        src/core/object/gc.h
        src/core/utils/sstring.h
        src/core/utils/guid.cpp
        src/core/utils/guid.h
        src/core/object/objectpointer.h
        src/core/meteor_core.h
        src/core/application/application.cpp
        src/core/application/application.h
        src/editor/app/editorapplication.cpp
        src/editor/app/editorapplication.h
        src/core/utils/logger.cpp
        src/core/utils/logger.h
        src/core/window/simple/windowing.cpp
        src/core/window/simple/windowing.h
        src/player/app/playerapplication.cpp
        src/player/app/playerapplication.h
        src/core/window/windowmanager.cpp
        src/core/window/windowmanager.h
        src/core/window/imgui/imguiwindow.cpp
        src/core/window/imgui/imguiwindow.h
        src/core/window/imgui/imguisubwindow.cpp
        src/core/window/imgui/imguisubwindow.h
        src/core/window/imgui/imguisubwindowmanager.cpp
        src/core/window/imgui/imguisubwindowmanager.h
        src/editor/editorwindows/console/editorconsolewindow.cpp
        src/editor/editorwindows/console/editorconsolewindow.h
        src/editor/meteorite.h
        src/core/window/imgui/imguiwindowconstants.h
        src/editor/editorwindows/hierarchy/editorhierarchywindow.cpp
        src/editor/editorwindows/hierarchy/editorhierarchywindow.h
        src/editor/editorwindows/inspector/editorinspectorwindow.cpp
        src/editor/editorwindows/inspector/editorinspectorwindow.h
        src/editor/editorwindows/scene/editorsceneviewwindow.cpp
        src/editor/editorwindows/scene/editorsceneviewwindow.cpp
        src/core/window/imgui/imguistyles.h
        src/core/utils/color.cpp
        src/core/utils/color.h
        src/core/utils/dpihelper.h
        src/core/graphics/core/graphicsrenderer.cpp
        src/core/graphics/core/graphicsrenderer.h
        src/core/graphics/core/sfmldrawcall.cpp
        src/core/graphics/core/sfmldrawcall.h
        src/core/graphics/core/drawcall.cpp
        src/core/graphics/core/drawcall.h
        src/core/graphics/core/opengldrawcall.cpp
        src/core/graphics/core/opengldrawcall.h
        src/core/utils/glmtypdefs.h
)

# FetchContent for dependencies
include(FetchContent)

set(SFML_VERSION 2.5.1)
set(IMGUI_VERSION 1.90.9)

# Fetch SFML
FetchContent_Declare(
        SFML
        URL "https://github.com/SFML/SFML/archive/${SFML_VERSION}.zip"
)

# Fetch Dear ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
        GIT_TAG "docking"
)

# Fetch ImGui-SFML binding
FetchContent_Declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG        2.6.x
)

# Fetch spdlog
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.12.0
)

# Fetch GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)


# Build SFML
option(SFML_BUILD_AUDIO "Build audio" OFF)
option(SFML_BUILD_NETWORK "Build network" OFF)
FetchContent_MakeAvailable(SFML)

# Build Dear ImGui
FetchContent_MakeAvailable(imgui)

# Build ImGui-SFML
set(IMGUI_DIR ${imgui_SOURCE_DIR})
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" ON)

FetchContent_MakeAvailable(imgui-sfml)
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(glm)


target_link_libraries(
        meteor_engine
        PUBLIC
        ImGui-SFML::ImGui-SFML
        spdlog::spdlog
        glm::glm
)

# Set platform-specific libraries
if (APPLE)
    # macOS requires CoreGraphics for DPI detection
    find_library(COREGRAPHICS_LIBRARY CoreGraphics)
    target_link_libraries(meteor_engine PRIVATE ${COREGRAPHICS_LIBRARY})
endif()

if (UNIX AND NOT APPLE)
    # Linux requires X11 for DPI detection
    find_package(X11 REQUIRED)
    target_link_libraries(meteor_engine PRIVATE X11)
endif()

# Set preprocessor definitions based on CMake options
if(BUILD_EDITOR)
    add_definitions(-DEDITOR_APPLICATION)
endif()

if(BUILD_PLAYER)
    add_definitions(-DPLAYER_APPLICATION)
endif()
