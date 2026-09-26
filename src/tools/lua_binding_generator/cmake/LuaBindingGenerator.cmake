# ---------------------------------------------------------------------------
# Lua Binding Generator
# ---------------------------------------------------------------------------
# Runs the C# binding generator as a pre-build step.
# If the generator finds validation errors (e.g. a SCRIPT_BIND_REF_PROP
# referencing a type that is not SCRIPT_BIND_CLASS), it exits non-zero
# and the build stops before the C++ compiler runs.
#
# Dependencies: .NET 8 SDK (dotnet CLI must be on PATH)
# ---------------------------------------------------------------------------

find_program(DOTNET_EXECUTABLE dotnet REQUIRED)

set(BINDING_GENERATOR_DIR  "${CMAKE_SOURCE_DIR}/src/tools/lua_binding_generator")
set(BINDING_OUTPUT_DIR     "${CMAKE_BINARY_DIR}/generated/bindings")
set(BINDING_STUB_DIR       "${BINDING_GENERATOR_DIR}/stubs")
set(BINDING_TOOL_PUBLISH   "${CMAKE_BINARY_DIR}/tools/binding-generator")
set(BINDING_TOOL_EXE       "${BINDING_TOOL_PUBLISH}/MeteorBindingGenerator.exe")

# ---------------------------------------------------------------------------
# Step 1: Publish the tool (only re-runs when tool source changes)
# ---------------------------------------------------------------------------
# Self-contained publish flattens all native deps (libclang, libClangSharp)
# into a single directory so the exe runs without DLL resolution issues.
# ---------------------------------------------------------------------------

file(GLOB_RECURSE BINDING_TOOL_SOURCES
        "${BINDING_GENERATOR_DIR}/src/*.cs"
        "${BINDING_GENERATOR_DIR}/LuaBindingGenerator.csproj"
)

add_custom_command(
        OUTPUT "${BINDING_TOOL_EXE}"
        COMMAND ${DOTNET_EXECUTABLE} publish
        "${BINDING_GENERATOR_DIR}/LuaBindingGenerator.csproj"
        -c Release -r win-x64 --self-contained
        --nologo -v quiet
        -o "${BINDING_TOOL_PUBLISH}"
        DEPENDS ${BINDING_TOOL_SOURCES}
        COMMENT "Building Lua binding generator..."
        VERBATIM
)

add_custom_target(BuildLuaBindingGenerator
        DEPENDS "${BINDING_TOOL_EXE}"
)

# ---------------------------------------------------------------------------
# Step 2: Run the tool (re-runs when any engine header changes)
# ---------------------------------------------------------------------------

file(GLOB_RECURSE BINDING_INPUT_HEADERS "${CMAKE_SOURCE_DIR}/src/*.h")

add_custom_command(
        OUTPUT "${BINDING_OUTPUT_DIR}/binding_registry.generated.h"
        COMMAND "${BINDING_TOOL_EXE}"
        "--source-dir" "${CMAKE_SOURCE_DIR}/src"
        "--output-dir" "${BINDING_OUTPUT_DIR}"
        "--stub-dir"   "${BINDING_STUB_DIR}"
        DEPENDS
        "${BINDING_TOOL_EXE}"
        ${BINDING_INPUT_HEADERS}
        COMMENT "Generating Lua bindings..."
        VERBATIM
)

add_custom_target(GenerateLuaBindings
        DEPENDS "${BINDING_OUTPUT_DIR}/binding_registry.generated.h"
)

# ---------------------------------------------------------------------------
# Wire up to engine targets
# ---------------------------------------------------------------------------

if (BUILD_EDITOR)
    add_dependencies(${EDITOR_NAME} GenerateLuaBindings)
    target_include_directories(${EDITOR_NAME} PRIVATE "${BINDING_OUTPUT_DIR}")
endif()

if (BUILD_PLAYER)
    add_dependencies(${PLAYER_NAME} GenerateLuaBindings)
    target_include_directories(${PLAYER_NAME} PRIVATE "${BINDING_OUTPUT_DIR}")
endif()