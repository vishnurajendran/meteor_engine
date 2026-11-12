# Meteor Engine

**Meteor** is an experimental 3D game engine and editor written in C++20 on top of OpenGL 4.6. It is a learning project, built to explore how engines work under the hood: render pipeline design, asset pipelines, physics and scripting integration, multithreading, and editor tooling.

> [!WARNING]
> Meteor is a **work in progress** and is **not ready for making games**. Expect breaking changes, missing features and rough edges.

![Meteorite editor](https://i.postimg.cc/BnStPZv6/Screenshot-2026-11-10-005517.png)

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
- Hybrid deferred, stage-based render pipeline: clear → prepass (depth, normals, surface) → shadows → skybox → opaque → lighting → resolve → SSAO → transparent → post-processing → composite → gizmos
- Linear HDR rendering, with sRGB encoding at the end
- Directional, point, spot and ambient lights (up to 32 dynamic lights), with a BVH for light lookup
- Cascaded shadow maps for directional lights, and spot / point light shadows with rolling updates
- Blinn-Phong shading with specular, normal maps, emission, alpha cutout, double-sided and transparent materials
- Post-processing stack: SSAO, fog with volumetric lighting, depth of field, bloom, FXAA and vignette
- Sprites, frustum culling, instanced drawing and debug views
- Procedural skybox
- Materials and a custom shader format (`.mesl`) that wraps GLSL vertex and fragment passes, with lit, toon and unlit templates

### Particles
- CPU-simulated particle emitters with a pooled particle buffer — no allocations while running
- Emission shapes with an editor gizmo, and per-axis (3D) start size
- Stackable modules: velocity (direction, drag), gravity, noise, and size / color / rotation over lifetime
- Curve and gradient editors for "over lifetime" values
- Sprite and mesh renderers, drawn with GPU instancing (one draw call per emitter)
- Alignment to the view, the velocity or the Z-axis

### Job System
- Engine-wide worker thread pool (`MJobSystem`)
- Fork-join `parallelFor` that is safe to nest — the calling thread helps with the work instead of waiting
- Used to simulate particle modules in parallel

### Physics — [Jolt Physics](https://github.com/jrouwe/joltphysics)
- Box, sphere, cylinder, capsule, mesh and convex hull collision shapes
- Raycasts
- Named physics layers

> Compound collision shapes are **not** supported yet.

### Audio — [MiniAudio](https://miniaud.io/)
- Audio sources and listeners
- 2D and spatial (3D) audio with roll-off, distance range and Doppler controls

### Scripting — Lua 5.4 + [sol2](https://github.com/ThePhD/sol2)
- Script behaviours attached to entities, with `onStart`, `onTick`, `onFixedTick`, `onLateTick` and `onStop` callbacks
- Engine bindings **generated automatically** from C++ headers at build time, including the full class hierarchy (casts and inherited methods work across every level)
- Generated [LuaLS](https://luals.github.io/) type definitions for IDE autocomplete

### Assets
- Model import via Assimp (glTF / GLB, OBJ, FBX), with import scale and pivot settings
- XML-based scene and asset serialisation (pugixml)
- `.meta` sidecar files that give each asset an ID
- Hot reload of changed assets (the file watcher runs on its own thread)

### Editor — Meteorite
- Dear ImGui interface with docking
- Scene viewport with transform gizmos (ImGuizmo)
- Hierarchy, inspector (with custom drawers per entity type), asset browser with thumbnails, and console
- Cut, copy and paste of entities
- Per-entity **Can Tick** toggle, saved with the scene
- Play mode inside the editor
- Project management: `.mtproj` project files, plus a WPF **launcher** for creating and opening projects
- Game publishing: packs the project into a standalone player with `.pak` asset archives
- Built-in profiler stats, memory window and editor settings

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

| Callback                  | When it runs                              |
|---------------------------|-------------------------------------------|
| `onStart(self)`           | Once, when play starts                    |
| `onTick(self, dt)`        | Every frame                               |
| `onFixedTick(self, dt)`   | Every fixed step (physics rate)           |
| `onLateTick(self, dt)`    | Every frame, after every `onTick`         |
| `onStop(self)`            | Once, when play stops                     |

> Tick callbacks only run while the entity's **Can Tick** option is on (the default). Turn it off in the inspector for entities that don't need per-frame updates.

To use a specific entity type, cast the entity by calling the type. The cast returns `nil` if the entity isn't that type. Casts work through the whole class hierarchy, so a point light can be cast to `MPointLight` and still use methods from its base classes:

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

**How the bindings are made.** Classes are exposed to Lua by annotating the C++ headers. At build time, the binding generator (`src/tools/lua_binding_generator`) parses those headers with libclang and generates the sol2 bindings. It also generates `meteor_api.lua`, the LuaLS type definitions, which are copied to `bin/.engine_data/scripting/symbols/lua/` for editor autocomplete.

---

## Project Layout

```
meteor_engine/
├── src/
│   ├── core/                  # Engine runtime (shared by editor and player)
│   │   ├── engine/            #   scene, entities, assets, physics, audio, scripting, lighting, input,
│   │   │                      #   jobs, particles, post-processing, skybox, sprites
│   │   ├── graphics/          #   render pipeline, stages, shaders, materials
│   │   ├── object/            #   object model, GC, type info
│   │   └── utils/             #   strings, math helpers, curves, parallel-for, logging, file IO
│   ├── editor/                # Meteorite editor (windows, inspectors, gizmos, project management, publishing)
│   ├── player/                # Standalone player application
│   └── tools/
│       ├── lua_binding_generator/   # C# tool: C++ headers → sol2 bindings + LuaLS defs
│       └── launcher/                # C# / WPF project launcher
├── post-build-copy-to-bin/    # Engine assets, shaders, fonts and templates copied to bin/ after build
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

GLEW and Assimp are built as shared libraries. Everything else is linked statically or is header-only. Assimp is built with only the glTF, OBJ and FBX importers; to support another format, turn on its `ASSIMP_BUILD_<FORMAT>_IMPORTER` option in `CMakeLists.txt`.

---

## Roadmap

**In progress**
- Editor UX and tooling polish
- Expanding the Lua API

**Planned**
- Animation system (skinning, built on the job system)
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
