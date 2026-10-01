# Meteor Engine

**Meteor** is an experimental 3D game engine and editor written in C++20 on top of OpenGL 4.6. It is a learning project, built to explore how engines work under the hood: render pipeline design, asset pipelines, physics and scripting integration, and editor tooling.

> [!WARNING]
> Meteor is a **work in progress** and is **not ready for making games**. Expect breaking changes, missing features and rough edges.

![Meteorite editor](https://i.postimg.cc/3R3zjZwK/Screenshot-2026-09-28-201652.png)

---

## Contents
- [Features](#features)
- [Getting Started](#getting-started)
- [Scripting](#scripting)
- [Project Layout](#project-layout)
- [Tech Stack](#tech-stack)
- [Roadmap](#roadmap)
- [License](#license)

---

## Features

### Rendering
- Modular, stage-based render pipeline: clear → depth → shadow → opaque → lighting → skybox → composite
- Directional, point, spot and ambient lights, with a BVH for light lookup
- Shadow mapping (early / experimental)
- Frustum culling
- Procedural skybox
- Materials and a custom shader format (`.mesl`) that wraps GLSL vertex and fragment passes, with lit, toon and unlit templates

### Physics — [Jolt Physics](https://github.com/jrouwe/joltphysics)
- Box, sphere, cylinder, capsule, mesh and convex hull collision shapes
- Raycasts
- Named physics layers

> Compound collision shapes are **not** supported yet.

### Audio — [MiniAudio](https://miniaud.io/)
- Audio sources and listeners
- 2D and spatial (3D) audio with roll-off, distance range and Doppler controls

### Scripting — Lua 5.4 + [sol2](https://github.com/ThePhD/sol2)
- Script behaviours attached to entities, with `onStart`, `onTick`, `onFixedTick` and `onStop` callbacks
- Engine bindings **generated automatically** from C++ headers at build time
- Generated [LuaLS](https://luals.github.io/) type definitions for IDE autocomplete

### Assets
- Model import via Assimp (glTF / GLB)
- XML-based scene and asset serialisation (pugixml)
- `.meta` sidecar files that give each asset an ID
- Hot reload of changed assets (the file watcher runs on its own thread)

### Editor — Meteorite
- Dear ImGui interface with docking
- Scene viewport with transform gizmos (ImGuizmo)
- Hierarchy, inspector (with custom drawers per entity type), asset browser with thumbnails, and console
- Play mode inside the editor
- Project management: `.mtproj` project files, plus a WPF **launcher** for creating and opening projects
- Built-in profiler stats and editor settings

---

## Getting Started

All C++ dependencies are downloaded and built by CMake (`FetchContent`), so nothing needs to be installed or configured by hand. Clone, configure, build.

> Build instructions may break as the project evolves.

### Requirements
- **Windows** with a **MinGW-w64 GCC** toolchain (C++20). Developed and tested with [MSYS2](https://www.msys2.org/) **UCRT64**.
- **CMake 3.26+** and **Ninja** (or CLion, which bundles both)
- **Git** (CMake uses it to fetch dependencies)
- **.NET 8 SDK**: builds the Lua binding generator, which runs during every build
- **.NET 9 SDK**: builds the project launcher (skip it with `-DMETEOR_BUILD_LAUNCHER=OFF`)

Install the toolchain from an MSYS2 UCRT64 shell:
```sh
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

### Build
```sh
git clone https://github.com/vishnurajendran/meteor_engine.git
cd meteor_engine
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```
Or open the folder in CLion with the MinGW (UCRT64) toolchain selected, and build.

The first configure and build take a few minutes, because every dependency is downloaded and compiled once. Later builds reuse them from the build folder.

### Output
Everything goes to `bin/`:

| File                                | What it is                                        |
|-------------------------------------|---------------------------------------------------|
| `Meteorite.exe`                     | The editor                                        |
| `MeteorPlayer.exe`                  | The standalone player                             |
| `MeteorLauncher.exe`                | Project launcher (if enabled)                     |
| `glew32*.dll`, `libassimp-*.dll`    | Shared dependencies, placed here automatically    |

### Running
Start `MeteorLauncher.exe` to create or open a project. The launcher opens the editor with that project (`Meteorite.exe --p <project>.mtproj`).

> **Running outside MSYS2:** the executables also need the GCC runtime DLLs (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`). Add `C:\msys64\ucrt64\bin` to your `PATH`, or copy those DLLs into `bin/`.

### Build Options
| Option                           | Default | Description                                               |
|----------------------------------|---------|-----------------------------------------------------------|
| `BUILD_EDITOR`                   | `ON`    | Build the editor (`Meteorite`)                            |
| `BUILD_PLAYER`                   | `ON`    | Build the player (`MeteorPlayer`)                         |
| `METEOR_BUILD_LAUNCHER`          | `ON`    | Build the WPF project launcher (needs .NET 9 SDK)         |
| `METEOR_LAUNCHER_SELF_CONTAINED` | `OFF`   | Bundle the .NET runtime into the launcher (~70 MB larger) |

---
## Scripting

A script is a Lua file that declares a class with `Behaviour:extend { ... }` and returns it. Fields and methods, including the lifecycle callbacks, are defined inside that table, and each method takes `self` as its first argument.

```lua
---@class Mover
local Mover = Behaviour:extend {
    speed = 5,

    onStart = function(self)
        MLogger.info("Mover attached to " .. self:myEntity():getName())
    end,

    onTick = function(self, dt)
        local entt = self:myEntity()
        if SInput.isDown(EKeyCode.W) then
            local newPos = entt:getWorldPosition() + entt:getForwardVector() * self.speed * dt
            entt:setWorldPosition(newPos)
        end
    end,

    onFixedTick = function(self, dt)
    end,

    onStop = function(self)
    end,
}

return Mover
```

To use a specific entity type, cast the entity by calling the type. The cast returns `nil` if the entity isn't that type:

```lua
onStart = function(self)
    local src = MAudioSource(self:myEntity())
    if src == nil then
        return
    end

    src:play()
end,
```

To talk to another script, find its entity and call `getScript()`:

```lua
local scene = MSceneManager.getSceneManagerInstance():getActiveScene()
local camera = MCameraEntity(scene:find("./MainCamera"))
if camera then
    camera:getScript():move(xAxis, yAxis)
end
```

| Callback                  | When it runs                              |
|---------------------------|-------------------------------------------|
| `onStart(self)`           | Once, when play starts                    |
| `onTick(self, dt)`        | Every frame                               |
| `onFixedTick(self, dt)`   | Every fixed step (physics rate)           |
| `onStop(self)`            | Once, when play stops                     |

**How the bindings are made.** Classes are exposed to Lua by annotating the C++ headers. At build time, the binding generator (`src/tools/lua_binding_generator`) parses those headers with libclang and generates the sol2 bindings. It also generates `meteor_api.lua`, the LuaLS type definitions, which are copied to `bin/.engine_data/scripting/symbols/lua/` for editor autocomplete.

---

| Callback          | When it runs                              |
|-------------------|-------------------------------------------|
| `onStart()`       | Once, when play starts                    |
| `onTick(dt)`      | Every frame                               |
| `onFixedTick(dt)` | Every fixed step (physics rate)           |
| `onStop()`        | Once, when play stops                     |

**How the bindings are made.** Classes are exposed to Lua by annotating the C++ headers. At build time, the binding generator (`src/tools/lua_binding_generator`) parses those headers with libclang and generates the sol2 bindings. It also generates `meteor_api.lua`, the LuaLS type definitions, which are copied to `bin/.engine_data/scripting/symbols/lua/` for editor autocomplete.

---

## Project Layout

```
meteor_engine/
├── src/
│   ├── core/                  # Engine runtime (shared by editor and player)
│   │   ├── engine/            #   scene, entities, assets, physics, audio, scripting, lighting, input
│   │   ├── graphics/          #   render pipeline, stages, shaders, materials
│   │   ├── object/            #   object model, GC, type info
│   │   └── utils/             #   strings, math helpers, logging, file IO
│   ├── editor/                # Meteorite editor (windows, inspectors, gizmos, project management)
│   ├── player/                # Standalone player application
│   └── tools/
│       ├── lua_binding_generator/   # C# tool: C++ headers → sol2 bindings + LuaLS defs
│       └── launcher/                # C# / WPF project launcher
├── post-build-copy-to-bin/    # Engine assets, shaders and templates copied to bin/ after build
└── CMakeLists.txt
```

---

## Tech Stack

| Category          | Technology                                                                                                   |
|-------------------|--------------------------------------------------------------------------------------------------------------|
| **Language**      | C++20                                                                                                        |
| **Rendering**     | OpenGL 4.6, [GLEW](https://glew.sourceforge.net/), [GLM](https://github.com/g-truc/glm), [stb](https://github.com/nothings/stb) |
| **Windowing**     | [SFML](https://www.sfml-dev.org/)                                                                            |
| **Editor UI**     | [Dear ImGui](https://github.com/ocornut/imgui), [ImGui-SFML](https://github.com/SFML/imgui-sfml), [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo), [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog) |
| **Physics**       | [Jolt Physics](https://github.com/jrouwe/joltphysics)                                                        |
| **Audio**         | [MiniAudio](https://miniaud.io/)                                                                             |
| **Scripting**     | [Lua 5.4](https://www.lua.org/), [sol2](https://github.com/ThePhD/sol2)                                      |
| **Model Loading** | [Assimp](https://www.assimp.org/)                                                                            |
| **Serialisation** | [pugixml](https://pugixml.org/), [Serialized Class](https://github.com/vishnurajendran/serialized_class)     |
| **Logging**       | [spdlog](https://github.com/gabime/spdlog)                                                                   |
| **Tooling**       | .NET 8 (binding generator, libclang), .NET 9 WPF (launcher)                                                  |

GLEW and Assimp are built as shared libraries. Everything else is linked statically or is header-only. Assimp is built with only the glTF importer; to support another format, turn on its `ASSIMP_BUILD_<FORMAT>_IMPORTER` option in `CMakeLists.txt`.

---

## Roadmap

**In progress**
- Render pipeline refinements (modular passes, shadows)
- Physics-based scene picking in the editor
- Asset system: dependency tracking
- Editor UX and tooling polish
- Expanding the Lua API

**Planned**
- Animation system
- Build pipeline and game packaging
- Plugin support
- Networking
- Compound collision shapes

---

## Goals
Meteor is primarily a **learning and experimentation** project, focused on:
- Modular rendering architecture
- Clean separation between engine and editor
- A scalable asset pipeline
- A tooling-first workflow

Performance and memory use are **not optimised yet**, and APIs change often as systems are rewritten.

---

## License
[MIT](LICENSE) © 2026 Vishnu Rajendran
