# Meteor Engine (Work in Progress)

Meteor is an experimental custom game engine built in C++20.  
It’s currently in active development, focused on exploring rendering architecture, tooling, and engine design.

> This project is **not ready for building games yet**. Expect breaking changes, missing systems and features, and rough edges.

---

## Current State (What Exists Right Now)

### Rendering
- Custom **scalable render pipeline (WIP refactor ongoing)**
- Basic forward rendering
- Procedural skybox
- Shadow support (early stage / experimental)

### Assets
- Model loading via **:contentReference[oaicite:0]{index=0}**
- XML-based asset serialization using **:contentReference[oaicite:1]{index=1}**
- Path-based asset referencing *(moving to Asset ID system)*

### Editor
- Immediate-mode editor built with **:contentReference[oaicite:2]{index=2}**
- Scene hierarchy (basic)
- Inspector (early)
- Scene viewport
- Debug tooling (in progress)

### Core
- Scene system with spatial entities
- Basic engine loop (Editor + Player separation)
- Plugin system (early design phase)

---

## In Progress / Actively Being Worked On

- Render pipeline rewrite (modular render passes)
- Scene picking (currently naive, will move to physics-based raycast)
- Asset system overhaul (Asset IDs, dependency tracking)
- Editor UX improvements (layout, tooling polish)

---

## Missing Systems (Not Implemented Yet)

- Physics system
- Scripting layer (planned DSL / runtime scripting)
- Audio system
- Animation system
- Build pipeline / packaging
- Extending engine with plugins
- Networking

---

## Tech Stack

- **Language:** C++20  
- **Rendering:** OpenGL
- **Windowing:** SFML, SFML-ImGui
- **GUI:** : ImGui
- **Model Loading:** : Assimp 
- **XML:** : pugixml

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
- Assimp
- GLEW
- SFML
- ImGui-SFML
- pugixml

*(Manual setup required depending on platform)*

---

## Current Editor Snapshot

![meteorite_editor](https://i.ibb.co/ccKXr9tC/Screenshot-2026-04-20-104203.png)

---

## Notes

- Editor performance and memory usage are **not optimized yet**
- APIs and systems are **subject to change frequently**
- Many systems are being rewritten as the architecture evolves

---

## License
MIT License
