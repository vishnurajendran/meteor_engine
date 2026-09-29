//
// meteor_paths.h
//
// The two roots every file path in the engine is relative to:
//
//   ENGINE_PATH   folder containing the running executable. Engine-owned,
//                 read-only data lives here: meteor_assets/, templates/,
//                 .engine_data/scripting/ (Lua modules + API symbols).
//
//   PROJECT_PATH  root of the open project: the folder holding
//                 <name>.mtproj. Project data lives here: assets/,
//                 .engine_data/settings/, editor caches, .vscode/.
//                 This is also made the process working directory, so
//                 existing relative paths resolve inside the project.
//
// Both return absolute, forward-slash paths. Pass a relative path to join it:
//     ENGINE_PATH("meteor_assets/splash.png")
//     PROJECT_PATH(".engine_data/settings")
//
// Command line (see SMeteorPaths::initialise):
//     meteorite.exe --p MyGame.mtproj      (relative to the working directory,
//                                           which the launcher sets to the project)
//     meteorite.exe --p D:/Games/MyGame/MyGame.mtproj
//     meteorite.exe                         (no project file: the working
//                                           directory is the project, as before)
//

#ifndef METEOR_PATHS_H
#define METEOR_PATHS_H

#include <filesystem>

#include "core/utils/sstring.h"

class SMeteorPaths
{
public:
    static constexpr const char* PROJECT_FILE_EXTENSION = ".mtproj";

    // Call once, first thing in main(). Resolves the engine root from the
    // executable, the project root from --p (or a single *.mtproj in the
    // working directory, or the working directory itself), then makes the
    // project root the working directory. Returns false if --p was given but
    // the file does not exist.
    static bool initialise(int argc, char** argv);

    [[nodiscard]] static const std::filesystem::path& engineRoot();
    [[nodiscard]] static const std::filesystem::path& projectRoot();

    // Absolute path of the .mtproj file; empty when running without one.
    [[nodiscard]] static const std::filesystem::path& projectFile();
    [[nodiscard]] static bool hasProjectFile() { return !projectFile().empty(); }

    // True when engine and project share a folder (running straight from the
    // build output without a separate project).
    [[nodiscard]] static bool isEngineProject();

    // Joins `relative` onto `root`. Absolute inputs are returned unchanged.
    [[nodiscard]] static SString join(const std::filesystem::path& root, const SString& relative);

    // For tests and tools: set the roots directly (does not change the
    // working directory).
    static void internal_setRoots(const std::filesystem::path& engine,
                                  const std::filesystem::path& project,
                                  const std::filesystem::path& projectFilePath = {});

private:
    static std::filesystem::path findExecutableDir(const char* argv0);
};

// Absolute path under the engine install (the executable's folder).
SString ENGINE_PATH(const SString& relative = "");

// Absolute path under the open project's root.
SString PROJECT_PATH(const SString& relative = "");

#endif // METEOR_PATHS_H
