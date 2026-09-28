cmake_minimum_required(VERSION 3.26)

project(meteor_engine)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/src)

option(BUILD_EDITOR "Build editor" ON)
option(BUILD_PLAYER "Build player" ON)

set(PLAYER_NAME MeteorPlayer)
set(EDITOR_NAME Meteorite)

include(FetchContent)

# ---------------------------------------------------------------------------
# SFML
# ---------------------------------------------------------------------------
set(SFML_VERSION 3.0.2)
set(IMGUI_VERSION 1.91.9-docking)
set(IMGUI_SFML_VERSION 3.0)

# It's nice to get stripped-down release zips instead of cloning
# a repo and checking out a tag
FetchContent_Declare(
        SFML
        URL "https://github.com/SFML/SFML/archive/${SFML_VERSION}.zip"
)

# ---------------------------------------------------------------------------
# IMGUI
# ---------------------------------------------------------------------------
FetchContent_Declare(
        imgui
        URL "https://github.com/ocornut/imgui/archive/v${IMGUI_VERSION}.zip"
)

# ---------------------------------------------------------------------------
# IMGUI-SFML
# ---------------------------------------------------------------------------
FetchContent_Declare(
        imgui-sfml
        URL "https://github.com/SFML/imgui-sfml/archive/v${IMGUI_SFML_VERSION}.zip"
)

## Build

# SFML
option(SFML_BUILD_AUDIO "Build audio" OFF)
option(SFML_BUILD_NETWORK "Build network" OFF)
FetchContent_MakeAvailable(sfml)

# Dear ImGui
FetchContent_MakeAvailable(imgui)

# ImGui-SFML
set(IMGUI_DIR ${imgui_SOURCE_DIR})
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" ON)
FetchContent_MakeAvailable(imgui-sfml)

# ---------------------------------------------------------------------------
# SERIALIZED-CLASS
# ---------------------------------------------------------------------------
FetchContent_Declare(
        serialized_class
        GIT_REPOSITORY  https://github.com/vishnurajendran/serialized_class.git
        GIT_TAG  master
)
FetchContent_MakeAvailable(serialized_class)

# ---------------------------------------------------------------------------
# SPDLOG
# ---------------------------------------------------------------------------
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)

# ---------------------------------------------------------------------------
# GLM
# ---------------------------------------------------------------------------
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# ---------------------------------------------------------------------------
# STB
# ---------------------------------------------------------------------------
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

# ---------------------------------------------------------------------------
# IMGUI-File Dialog  (aiekick/ImGuiFileDialog)
# ---------------------------------------------------------------------------
# The library has its own CMakeLists.txt that produces the ImGuiFileDialog
# target, but its find_package(imgui) check fails in a FetchContent setup.
# We point it at our already-fetched imgui source before MakeAvailable so
# the include path resolves correctly.
set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_Declare(
        imgui-file-dialog
        GIT_REPOSITORY "https://github.com/aiekick/ImGuiFileDialog.git"
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui-file-dialog)

# Ensure imgui headers are visible even if the library's find_package missed them.
target_include_directories(ImGuiFileDialog PUBLIC ${imgui_SOURCE_DIR})

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# ---------------------------------------------------------------------------
# IMGUIZMO
# ---------------------------------------------------------------------------
FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imguizmo)

# ImGuizmo now ships its own CMakeLists.txt that creates the target automatically.
# We just patch the existing target with the extra includes/links we need.
target_include_directories(imguizmo PUBLIC
        ${imguizmo_SOURCE_DIR}
        ${imgui_SOURCE_DIR}
)
target_link_libraries(imguizmo PRIVATE ImGui-SFML::ImGui-SFML)

# ---------------------------------------------------------------------------
# PUGIXML
# ---------------------------------------------------------------------------
FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG v1.15
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(pugixml)

# ---------------------------------------------------------------------------
# Vendor Dependencies (GLEW + ASSIMP...)
# ---------------------------------------------------------------------------

# BUILD_SHARED_LIBS is a global switch, so it is turned on only for these two
# fetches and restored afterwards - otherwise Jolt, Lua etc. would flip too.
set(_METEOR_SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS ON)

# ---- GLEW (official release tarball) ---------------------------------------
# The GLEW git repo cannot be built directly - its sources are generated at
# release time - so we use the release tarball, which ships a CMake project
# under build/cmake.
set(GLEW_VERSION 2.2.0)
set(BUILD_UTILS OFF CACHE BOOL "" FORCE) # skip glewinfo / visualinfo

# GLEW's CMakeLists asks for cmake_minimum_required(2.8.12), Ignored by CMake 3.x.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
        glew
        URL "https://github.com/nigels-com/glew/releases/download/glew-${GLEW_VERSION}/glew-${GLEW_VERSION}.tgz"
        URL_HASH SHA256=d4fc82893cfb00109578d0a1a2337fb8ca335b3ceccf97b97e5cc7f08e4353e1
        SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(glew)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# GLEW only exports its include dir for installed builds, not in-tree ones.
target_include_directories(glew INTERFACE $<BUILD_INTERFACE:${glew_SOURCE_DIR}/include>)
# GLEW overrides the output dir to its own build folder — put glew32.dll in bin/.
set_target_properties(glew PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# GLEW always defines a static variant too — we don't use it, so don't build it.
set_target_properties(glew_s PROPERTIES EXCLUDE_FROM_ALL TRUE)

# ---- ASSIMP (glTF / GLB only) -----------------------------------------------
set(ASSIMP_BUILD_TESTS                   OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS            OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES                 OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL                       OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT                     ON  CACHE BOOL "" FORCE) # import only
set(ASSIMP_WARNINGS_AS_ERRORS            OFF CACHE BOOL "" FORCE) # newer GCC warns more
set(ASSIMP_INJECT_DEBUG_POSTFIX          OFF CACHE BOOL "" FORCE) # stops a global "d" postfix leaking out
set(ASSIMP_IGNORE_GIT_HASH               ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB                    ON  CACHE BOOL "" FORCE) # bundled zlib — no system dependency

# Disable every importer, then opt in to the formats we support.
# glTF covers both .gltf and .glb. To add a format later, add a line —
# e.g. ASSIMP_BUILD_FBX_IMPORTER / ASSIMP_BUILD_OBJ_IMPORTER.
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER           ON  CACHE BOOL "" FORCE)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.2
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(assimp)

# Restore — everything fetched below this point is static again.
set(BUILD_SHARED_LIBS ${_METEOR_SAVED_BUILD_SHARED_LIBS})
unset(_METEOR_SAVED_BUILD_SHARED_LIBS)

# The force-include below applies to every target defined after it — including
# miniaudio, Jolt and Lua — so they all need to be able to find GL/glew.h.
# Previously the global MSYS2 include path provided that.
include_directories(${glew_SOURCE_DIR}/include)

# ensure glew is first header
if (MINGW OR WIN32)
    add_compile_options(-include GL/glew.h)
endif()

# ---------------------------------------------------------------------------
# Mini Audio
# ---------------------------------------------------------------------------
FetchContent_Declare(
        miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG 0.11.25
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniaudio)

#----------------------------------------------------------------------------
# Jolt Physics
#----------------------------------------------------------------------------

set(TARGET_UNIT_TESTS       OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD      OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES          OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER           OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.5.0
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR Build
)

FetchContent_MakeAvailable(JoltPhysics)

#----------------------------------------------------------------------------
# Lua
#----------------------------------------------------------------------------
FetchContent_Declare(
        lua
        URL "https://www.lua.org/ftp/lua-5.4.7.tar.gz"
)
FetchContent_MakeAvailable(lua)

set(LUA_SRC_DIR ${lua_SOURCE_DIR}/src)

add_library(lua_static STATIC
        ${LUA_SRC_DIR}/lapi.c
        ${LUA_SRC_DIR}/lauxlib.c
        ${LUA_SRC_DIR}/lbaselib.c
        ${LUA_SRC_DIR}/lcode.c
        ${LUA_SRC_DIR}/lcorolib.c
        ${LUA_SRC_DIR}/lctype.c
        ${LUA_SRC_DIR}/ldblib.c
        ${LUA_SRC_DIR}/ldebug.c
        ${LUA_SRC_DIR}/ldo.c
        ${LUA_SRC_DIR}/ldump.c
        ${LUA_SRC_DIR}/lfunc.c
        ${LUA_SRC_DIR}/lgc.c
        ${LUA_SRC_DIR}/linit.c
        ${LUA_SRC_DIR}/liolib.c
        ${LUA_SRC_DIR}/llex.c
        ${LUA_SRC_DIR}/lmathlib.c
        ${LUA_SRC_DIR}/lmem.c
        ${LUA_SRC_DIR}/loadlib.c
        ${LUA_SRC_DIR}/lobject.c
        ${LUA_SRC_DIR}/lopcodes.c
        ${LUA_SRC_DIR}/loslib.c
        ${LUA_SRC_DIR}/lparser.c
        ${LUA_SRC_DIR}/lstate.c
        ${LUA_SRC_DIR}/lstring.c
        ${LUA_SRC_DIR}/lstrlib.c
        ${LUA_SRC_DIR}/ltable.c
        ${LUA_SRC_DIR}/ltablib.c
        ${LUA_SRC_DIR}/ltm.c
        ${LUA_SRC_DIR}/lundump.c
        ${LUA_SRC_DIR}/lutf8lib.c
        ${LUA_SRC_DIR}/lvm.c
        ${LUA_SRC_DIR}/lzio.c
)

target_include_directories(lua_static PUBLIC ${LUA_SRC_DIR})

# Lua needs math and dynamic-loading libs on Unix
if(UNIX)
    target_link_libraries(lua_static PRIVATE m dl)
endif()


#----------------------------------------------------------------------------
# Sol2
#----------------------------------------------------------------------------
FetchContent_Declare(
        Sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG        v3.3.0
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(sol2)
add_compile_options(-Wno-template-body)

if (MINGW)
    add_compile_options(-Wa,-mbig-obj)
endif()

# ---------------------------------------------------------------------------
# Source file collection
# ---------------------------------------------------------------------------
# GLOB_RECURSE replaces the old manual file lists.
#
# Tradeoff: newly added or deleted files are not detected until you
# re-run cmake configure (cmake ..). The CMAKE_CONFIGURE_DEPENDS
# property below mitigates this by forcing a re-configure whenever
# the directory contents change.
# ---------------------------------------------------------------------------

file(GLOB_RECURSE CORE_SOURCES
        "${CMAKE_SOURCE_DIR}/src/core/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/core/*.h"
)

# Files at src/ root that are not under src/core/
list(APPEND CORE_SOURCES
        src/main.cpp
        src/default_engine_icon_paths.h
)

file(GLOB_RECURSE EDITOR_SOURCES
        "${CMAKE_SOURCE_DIR}/src/editor/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/editor/*.h"
)

file(GLOB_RECURSE PLAYER_SOURCES
        "${CMAKE_SOURCE_DIR}/src/player/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/player/*.h"
)

# Force CMake to re-configure when files are added or removed
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/core"
        "${CMAKE_SOURCE_DIR}/src/editor"
        "${CMAKE_SOURCE_DIR}/src/player"
)

# ---------------------------------------------------------------------------
# Editor
# ---------------------------------------------------------------------------
if (BUILD_EDITOR)

    add_compile_definitions(
            METEOR_EDITOR=1
    )

    add_executable( ${EDITOR_NAME}

            ${CORE_SOURCES}
            ${EDITOR_SOURCES}

            resource.rc
    )

    target_compile_definitions(${EDITOR_NAME} PRIVATE EDITOR_APPLICATION)

    target_link_libraries(${EDITOR_NAME} PRIVATE
            SFML::Graphics
            SFML::Window
            SFML::System

            ImGui-SFML::ImGui-SFML
            spdlog::spdlog
            glm::glm
            stb
            imguizmo

            glew
            assimp::assimp
            pugixml
            serialized_class
            ImGuiFileDialog
            miniaudio
            Jolt
            lua_static
            sol2
    )
    target_link_options(${EDITOR_NAME} PRIVATE "-Wl,--allow-multiple-definition")
endif()

# ---------------------------------------------------------------------------
# Player
# ---------------------------------------------------------------------------
if (BUILD_PLAYER)
    add_compile_definitions(
            METEOR_PLAYER=1
    )

    add_executable( ${PLAYER_NAME}
            ${CORE_SOURCES}
            ${PLAYER_SOURCES}

            resource.rc
    )

    target_compile_definitions(${PLAYER_NAME} PRIVATE PLAYER_APPLICATION)

    target_link_libraries(${PLAYER_NAME} PRIVATE
            SFML::Graphics
            SFML::Window
            SFML::System

            ImGui-SFML::ImGui-SFML
            spdlog::spdlog
            glm::glm
            stb

            glew
            assimp::assimp
            pugixml
            serialized_class
            miniaudio
            Jolt
            lua_static
            sol2
    )

    target_link_options(${PLAYER_NAME} PRIVATE "-Wl,--allow-multiple-definition")
endif()

# ---------------------------------------------------------------------------
# Lua Binding Generator (must come AFTER targets are defined)
# ---------------------------------------------------------------------------
include(src/tools/lua_binding_generator/cmake/LuaBindingGenerator.cmake)

# ---------------------------------------------------------------------------
# Meteor Launcher (WPF, Windows). Uses DOTNET_EXECUTABLE from above and
# copies MeteorLauncher.exe into bin/ next to the editor.
# ---------------------------------------------------------------------------
include(src/tools/launcher/cmake/MeteorLauncher.cmake)

# ---------------------------------------------------------------------------
# Common Defines
# ---------------------------------------------------------------------------
add_compile_definitions(
        #VERBOSE_LOGGING=1
)


# ---------------------------------------------------------------------------
# Platform-specific
# ---------------------------------------------------------------------------
if (APPLE)
    find_library(COREGRAPHICS_LIBRARY CoreGraphics)
    target_link_libraries(${EDITOR_NAME} PRIVATE ${COREGRAPHICS_LIBRARY})
endif()

if (UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(${EDITOR_NAME} PRIVATE X11)
endif()

# ---------------------------------------------------------------------------
# Copy system
# ---------------------------------------------------------------------------

set(DEST_COPY_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
function(add_copy_post_build target_name source_dir)
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying assets for ${target_name}..."

            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${source_dir}"
            "${DEST_COPY_DIR}"

            VERBATIM
    )
endfunction()

set(COMMON_SOURCE_COPY_DIR "${CMAKE_SOURCE_DIR}/post-build-copy-to-bin/common")
set(EDITOR_SOURCE_COPY_DIR "${CMAKE_SOURCE_DIR}/post-build-copy-to-bin/editor")


if (BUILD_EDITOR)
    add_copy_post_build(${EDITOR_NAME} ${COMMON_SOURCE_COPY_DIR})
    add_copy_post_build(${EDITOR_NAME} ${EDITOR_SOURCE_COPY_DIR})
endif()

if (BUILD_PLAYER)
    add_copy_post_build(${PLAYER_NAME} ${COMMON_SOURCE_COPY_DIR})
endif()

# ---------------------------------------------------------------------------
# Copy meteor_api.lua to the runtime symbols directory for IDE intellisense
# ---------------------------------------------------------------------------
set(SYMBOLS_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.engine_data/scripting/symbols/lua")

add_custom_command(
        TARGET GenerateLuaBindings POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SYMBOLS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${BINDING_OUTPUT_DIR}/meteor_api.lua"
        "${SYMBOLS_DIR}/meteor_api.lua"
        COMMENT "Copying meteor_api.lua to .engine_data/scripting/symbols/lua"
        VERBATIM
)