
# Meteor Game Engine (Work in Progress)

Meteor is a custom game engine built in C++20, featuring a player and an editor. It leverages modern C++ and various libraries to provide a robust and flexible environment for game development.

![meteorite_editor](https://i.ibb.co/7gw1xLP/Screenshot-2024-10-06-134159.png)

![meteor_player](https://i.ibb.co/JnkVbsq/Screenshot-2024-10-06-133554.png)

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

### Installation

#### 1. Installing Assimp

To install Assimp on your system, follow these steps:

- **Linux (Ubuntu/Debian):**
  ```bash
  sudo apt-get update
  sudo apt-get install libassimp-dev
  ```

- **macOS:**
  ```bash
  brew install assimp
  ```

- **Windows:**
  1. Download the Assimp binaries from the [Assimp GitHub Releases](https://github.com/assimp/assimp/releases).
  2. Extract the contents and add the \`include\` and \`lib\` directories to your CMake configuration:
     - Add the path to the \`include\` directory using \`include_directories\`.
     - Ensure the \`lib\` path is available in the \`CMakeLists.txt\` or your compiler’s library path.

#### 2. Installing GLEW

To install GLEW, follow these steps:

- **Linux (Ubuntu/Debian):**
  ```bash
  sudo apt-get update
  sudo apt-get install libglew-dev
  ```

- **macOS:**
  ```bash
  brew install glew
  ```

- **Windows:**
  1. Download the GLEW binary package from the [GLEW website](http://glew.sourceforge.net/).
  2. Extract the downloaded ZIP file to a directory (e.g., `C:\glew`).
  3. Add the `include` path (`C:\glew\include`) and the library path (`C:\glew\lib\Release\x64`) in CMake:
     - Add the path to the `include` directory using `include_directories`.
     - Add the path to the `lib` directory using `link_directories`.

### License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
