# ---------------------------------------------------------------------------
# Meteor Launcher (C# / WPF, Windows only)
# ---------------------------------------------------------------------------
# Publishes src/tools/launcher/MeteorLauncher as a single MeteorLauncher.exe
# and copies it next to the editor in CMAKE_RUNTIME_OUTPUT_DIRECTORY (bin/).
#
# The editor starts the launcher when it is opened without a project
# (see main.cpp), and the launcher starts Meteorite.exe --p <project>.mtproj.
#
# Dependencies: .NET 9 SDK (dotnet on PATH; DOTNET_EXECUTABLE is found by
# LuaBindingGenerator.cmake, which must be included first).
#
# Options:
#   METEOR_BUILD_LAUNCHER           build and copy the launcher (default ON)
#   METEOR_LAUNCHER_SELF_CONTAINED  bundle the .NET runtime into the exe
#                                   (~70 MB). OFF needs the .NET 9 Desktop
#                                   Runtime on the machine (the SDK has it).
# ---------------------------------------------------------------------------

option(METEOR_BUILD_LAUNCHER          "Build the WPF project launcher"               ON)
option(METEOR_LAUNCHER_SELF_CONTAINED "Publish the launcher with the .NET runtime"   OFF)

if (NOT WIN32 OR NOT BUILD_EDITOR OR NOT METEOR_BUILD_LAUNCHER)
    return()
endif()

if (NOT DOTNET_EXECUTABLE)
    find_program(DOTNET_EXECUTABLE dotnet REQUIRED)
endif()

set(LAUNCHER_PROJECT_DIR "${CMAKE_SOURCE_DIR}/src/tools/launcher/MeteorLauncher")
set(LAUNCHER_PUBLISH_DIR "${CMAKE_BINARY_DIR}/tools/launcher")
set(LAUNCHER_EXE_NAME    "MeteorLauncher.exe")
set(LAUNCHER_PUBLISH_EXE "${LAUNCHER_PUBLISH_DIR}/${LAUNCHER_EXE_NAME}")

# Rebuild when any source, XAML, resource or the project file changes.
# bin/ and obj/ hold dotnet's own build output and must not be dependencies.
file(GLOB_RECURSE LAUNCHER_SOURCES
        "${LAUNCHER_PROJECT_DIR}/*.cs"
        "${LAUNCHER_PROJECT_DIR}/*.xaml"
        "${LAUNCHER_PROJECT_DIR}/*.csproj"
        "${LAUNCHER_PROJECT_DIR}/res/*"
)
list(FILTER LAUNCHER_SOURCES EXCLUDE REGEX "/(bin|obj)/")

if (METEOR_LAUNCHER_SELF_CONTAINED)
    set(LAUNCHER_SELF_CONTAINED_ARG --self-contained true)
else()
    set(LAUNCHER_SELF_CONTAINED_ARG --self-contained false)
endif()

add_custom_command(
        OUTPUT "${LAUNCHER_PUBLISH_EXE}"
        COMMAND ${DOTNET_EXECUTABLE} publish
        "${LAUNCHER_PROJECT_DIR}/MeteorLauncher.csproj"
        -c Release -r win-x64 ${LAUNCHER_SELF_CONTAINED_ARG}
        -p:PublishSingleFile=true
        -p:IncludeNativeLibrariesForSelfExtract=true
        --nologo -v quiet
        -o "${LAUNCHER_PUBLISH_DIR}"
        DEPENDS ${LAUNCHER_SOURCES}
        COMMENT "Building Meteor Launcher..."
        VERBATIM
)

add_custom_target(BuildMeteorLauncher
        DEPENDS "${LAUNCHER_PUBLISH_EXE}"
)

# Copy into bin/ next to the editor. copy_if_different keeps this cheap on
# every build. (Fails only if the launcher is running at that moment.)
add_custom_command(
        TARGET BuildMeteorLauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LAUNCHER_PUBLISH_EXE}"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${LAUNCHER_EXE_NAME}"
        COMMENT "Copying ${LAUNCHER_EXE_NAME} to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        VERBATIM
)

add_dependencies(${EDITOR_NAME} BuildMeteorLauncher)
