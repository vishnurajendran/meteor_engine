
# Meteor Game Engine (Work in Progress)

Meteor is a custom game engine built in C++20, featuring a player and an editor. It leverages modern C++ and various libraries to provide a robust and flexible environment for game development.

![meteorite_editor](https://i.ibb.co/WSWkbc7/Screenshot-2025-02-13-131919.png)

## Features

- Built with **C++20** using the **STL**.
- Uses **OpenGL** for rendering and **GLEW** for OpenGL extensions.
- Supports model loading with **Assimp**.
- Incorporates **ImGui** for creating user interfaces in the editor.
- Integrated with **SFML** for windowing and input handling.
- XML parsing with **pugixml**.

## Getting Started

### Prerequisites

- **C++20** compatible compiler (e.g., GCC, Clang, MSVC).
- **CMake** (version 3.26 or higher) for project configuration.
- **CLion** (optional) for an integrated development environment.

### Dependencies

- [**pugixml**](https://pugixml.org/)
- [**Assimp**](https://github.com/assimp/assimp)
- [**GLEW**](http://glew.sourceforge.net/)
- [**OpenGL**](https://www.opengl.org/)
- [**SFML**](https://www.sfml-dev.org/)
- [**imgui-sfml**](https://github.com/SFML/imgui-sfml)

### Installation (Windows)

#### 1. Installing Assimp (MSYS2)

run this command in the MSYS2 console window: ```bash pacman -S mingw-w64-x86_64-assimp```

#### 2. Installing GLEW (MSYS2)

run this command in the MSYS2 console window: ```bash pacman -S mingw-w64-x86_64-glew```

### 3. Change the lines 105 and 88 in CMakeLists.txt to point the include directory within the MSYS2 installation.


### License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
