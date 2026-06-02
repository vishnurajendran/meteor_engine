[![Doxygen to GitHub Pages](https://github.com/vishnurajendran/meteor_engine/actions/workflows/doxygen.yml/badge.svg)](https://github.com/vishnurajendran/meteor_engine/actions/workflows/doxygen.yml)

# Meteor Engine (Work in Progress)

Meteor is an experimental custom game engine built in C++20.  
It’s currently in active development, focused on exploring rendering architecture, tooling, and engine design.

> This project is **not ready for building games yet**. Expect breaking changes, missing systems and features, and rough edges.
&nbsp;

---

## Current State (What Exists Right Now)
![meteorite_editor_snapshot](https://i.ibb.co/dw66j6YK/Screenshot-2026-06-01-013540.png)
![meteorite_editor_gif](https://i.ibb.co/mC2ZqqQs/meteor.gif)

### Core
- Scene system with spatial entities
- Basic engine loop (Editor + Player separation)
- Audio engine powered by MiniAudio, supports 2D and spatial audio
- Physics engine powered by Jolt Physics
  - supports Box, Sphere, Cylinder, Capsule, Mesh and Convex Hull collision shapes.
  - Raycasts.
  - Named Physics layers.
> **NOTE:** Currently compound collision shapes are **NOT** supported

### Rendering
- Custom **scalable render pipeline (WIP refactor ongoing)**
- Basic forward+ rendering
- Procedural skybox
- Shadow support (early stage / experimental)

### Assets
- Model loading via Assimp
- XML-based asset serialization using **pugixml**
- Path-based asset referencing *(moving to Asset ID system)*

### Editor
- Immediate-mode editor built with **Imgui, SFML-Imgui**
- Scene hierarchy (basic)
- Inspector (early)
- Scene viewport
- Custom inspectors
---

## In Progress / Actively Being Worked On

- Render pipeline rewrite (modular render passes)
- Scene picking (currently naive, will move to physics-based raycast)
- Asset system overhaul (Asset IDs, dependency tracking)
- Editor UX improvements (layout, tooling polish)

---

## Missing/Pending Systems (Not Implemented Yet)

- Scripting layer (planned DSL / runtime scripting)
- Animation system
- Build pipeline / packaging
- Extending engine with plugins
- Networking

---
## Tech Stack

| Category          | Technology                                                                                               |
|-------------------|----------------------------------------------------------------------------------------------------------|
| **Language**      | [C++20](https://en.cppreference.com/cpp/20)                                                              |
| **Rendering**     | [OpenGL v4.6](https://www.opengl.org/)                                                                   |
| **Windowing**     | [SFML](https://www.sfml-dev.org/), [SFML-ImGui](https://github.com/SFML/imgui-sfml)                      |
| **Audio**         | [MiniAudio](https://miniaud.io/index.html)                                                               |
| **Physics**       | [Jolt Physics](https://github.com/jrouwe/joltphysics)                                                    |
| **GUI**           | [ImGui](https://github.com/ocornut/imgui)                                                                |
| **Model Loading** | [Assimp](https://www.assimp.org/)                                                                        |
| **Serlization**   | [PugiXML](https://pugixml.org/), [Serialized Class](https://github.com/vishnurajendran/serialized_class) |

---

## Goals
Meteor is primarily a **learning + experimentation project**, with focus on:
- Modular rendering architecture
- Clean engine/editor separation
- Scalable asset pipeline
- Tooling-first workflow

---

## Building the Project
> Build instructions may break as the project evolves.

### Requirements
- C++20 compiler
- CMake (3.26+)

### Dependencies
- Assimp*
- GLEW*
- SFML
- ImGui-SFML
- pugixml
- Serialized Class
- Mini Audio
- Jolt Physics

*\*(Manual setup required depending on platform)*

---
## Notes
- Editor performance and memory usage are **not optimized yet**
- APIs and systems are **subject to change frequently**
- Many systems are being rewritten as the architecture evolves

---
## License
MIT License
