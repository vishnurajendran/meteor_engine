# Meteor Engine

**Meteor** is an experimental 3D game engine and editor written in C++20 on top of OpenGL 4.6. It is a learning project, built to explore how engines work under the hood: render pipeline design, asset pipelines, physics and scripting integration, multithreading, and editor tooling.

You build a game in the **Meteorite** editor, script it in **Lua**, and publish a standalone build that runs on **MeteorPlayer**.

> [!WARNING]
> Meteor is a **work in progress** and is **not ready for making games**. Expect breaking changes, missing features and rough edges.

![Meteorite editor](https://i.postimg.cc/BnStPZv6/Screenshot-2026-11-10-005517.png)

## Documentation

Everything lives in the **[wiki](https://github.com/vishnurajendran/meteor_engine/wiki)**. It is both the getting-started guide and the tech bible for the engine.

| Start here | |
|---|---|
| [Getting Started](https://github.com/vishnurajendran/meteor_engine/wiki/Getting-Started) | Toolchain, building, running, your first project |
| [Editor Guide](https://github.com/vishnurajendran/meteor_engine/wiki/Guide-Editor) | Projects, the editor, building a scene, play mode |
| [Scripting Guide](https://github.com/vishnurajendran/meteor_engine/wiki/Guide-Scripting) | Writing Lua behaviours |
| [Architecture Overview](https://github.com/vishnurajendran/meteor_engine/wiki/Architecture-Overview) | How the engine is put together |
| [Features](https://github.com/vishnurajendran/meteor_engine/wiki/Features) / [Roadmap](https://github.com/vishnurajendran/meteor_engine/wiki/Roadmap) | What works today, and what's next |

## Features

- **Rendering** — hybrid deferred pipeline on OpenGL 4.6, linear HDR, directional/point/spot lights with cascaded and cached shadows, transparents, post-processing (SSAO, fog with volumetrics, depth of field, bloom, FXAA, vignette), sprites, procedural and cubemap skies, a custom `.mesl` shader format with compute shaders.
- **Animation** — skeletal meshes and animation-only files, crossfades, culling modes, bone sockets and look-at bone controls, a scriptable pose hook, GPU compute skinning with a CPU fallback.
- **Game UI** — [RmlUi](https://github.com/mikke89/RmlUi) widgets (`.rml` + `.rcss` + Lua behaviours), nested widgets, events, fonts, hot reload.
- **Particles** — CPU-simulated, GPU-instanced emitters with stackable modules and curve/gradient editors.
- **Physics** — [Jolt Physics](https://github.com/jrouwe/joltphysics): six shapes, static/dynamic/kinematic bodies, layers, triggers, collision events, raycasts.
- **Audio** — [miniaudio](https://miniaud.io/): 2D and spatial sources, streaming, Doppler.
- **Scripting** — Lua 5.4 + [sol2](https://github.com/ThePhD/sol2), with engine bindings generated from C++ headers and LuaLS autocomplete in VS Code.
- **Input** — keyboard, mouse and cursor lock.
- **Assets** — GUID-based references, importers for models (glTF, OBJ, FBX), textures, audio and more, hot reload.
- **Editor** — docking ImGui editor with scene view and gizmos, hierarchy, inspectors, asset browser, console, profiler and memory windows, buffer views, prefabs (compositions), play mode and one-click publishing.
- **Engine core** — subsystem registry, pooled memory allocator with tracking, fork-join job system.

The full list is on the [Features](https://github.com/vishnurajendran/meteor_engine/wiki/Features) page.

## Quick start

Requires **Windows**, **MSYS2 UCRT64** (`gcc`, `cmake`, `ninja`), **Git**, the **.NET 8 SDK** (the Lua binding generator runs on every build) and optionally the **.NET 9 SDK** (the launcher; skip it with `-DMETEOR_BUILD_LAUNCHER=OFF`). CMake fetches every C++ dependency itself.

```sh
git clone https://github.com/vishnurajendran/meteor_engine.git
cd meteor_engine
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

Everything is built into `bin/`. Start `bin/MeteorLauncher.exe`, create a project, and it opens in the editor.

> [!NOTE]
> Outside an MSYS2 shell, the executables also need the GCC runtime DLLs (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`). Add `C:\msys64\ucrt64\bin` to your `PATH` or copy them next to the executables.

Build options, output layout and troubleshooting: [Getting Started](https://github.com/vishnurajendran/meteor_engine/wiki/Getting-Started).

## A taste of scripting

```lua
local Spinner = CLASS("Spinner", Behaviour) {
    degreesPerSecond = 90,          -- shows in the Inspector, tunable per entity

    onTick = function(self, dt)
        local e = self:myEntity()
        local turn = SQuaternion.fromEuler(0, self.degreesPerSecond * dt, 0)
        e:setRelativeRotation(turn * e:getRelativeRotation())
    end,
}

return Spinner
```

More in the [Scripting Guide](https://github.com/vishnurajendran/meteor_engine/wiki/Guide-Scripting).

## Tech stack

C++20 (MinGW-w64 GCC) · OpenGL 4.6, GLEW, GLM, stb · SFML 3 · Dear ImGui, ImGui-SFML, ImGuizmo, ImGuiFileDialog · RmlUi · Jolt Physics · miniaudio · Lua 5.4 + sol2 · Assimp · pugixml, [Serialized Class](https://github.com/vishnurajendran/serialized_class) · spdlog · .NET 8 / 9 for tooling

Versions and how each is built: [Build System](https://github.com/vishnurajendran/meteor_engine/wiki/Build-System).

## Goals

Meteor is primarily a **learning and experimentation** project, focused on a modular rendering architecture, clean separation between engine and editor, a scalable asset pipeline and a tooling-first workflow. Performance and memory use are not fully optimised yet, and APIs change as systems are rewritten.

## License

[MIT](LICENSE) © 2026 Vishnu Rajendran
