//
// editor_project_manager.h
//
// Owns the open project for the editor session.
//
// The launcher creates the project folder and its .mtproj, sets the working
// directory to it and starts the editor with --p <file>. SMeteorPaths has
// already resolved ENGINE_PATH / PROJECT_PATH by the time this runs; this
// class then:
//
//   prepare()           (before settings load)
//       reads the .mtproj and makes sure the project layout exists:
//       folders, engine scripting data synced from the engine install,
//       .vscode/settings.json if missing. Safe to run on every open; it
//       only creates what is missing and refreshes engine-owned copies.
//
//   applyToAssetManager()   (before the first asset refresh)
//       gives the asset manager a source over the project + engine install,
//       adds <project>/templates to the template registry and sets
//       __PROJECT_NAME__.
//

#ifndef EDITOR_PROJECT_MANAGER_H
#define EDITOR_PROJECT_MANAGER_H

#include <cstdint>
#include <limits>

#include "core/project/project_descriptor.h"

class MEditorAssetManager;

class MEditorProjectManager
{
public:
    // Engine-owned data copied into each project so VS Code (LuaLS) can use
    // relative paths. The engine itself always reads the ENGINE_PATH copy.
    static constexpr const char* ENGINE_SCRIPTING_DIR = ".engine_data/scripting";

    // Per-project folders created on open (relative to PROJECT_PATH).
    static constexpr const char* PROJECT_TEMPLATES_DIR = "templates";

    // Generated per project: every script's class (base, cast, members) and
    // one typed import() overload per script, so LuaLS autocompletes names
    // and members. Lives under the symbols folder VS Code already loads as a
    // library.
    static constexpr const char* SCRIPT_INDEX_FILE = ".engine_data/scripting/symbols/lua/script_index.lua";

    bool prepare();
    void applyToAssetManager(MEditorAssetManager& assetManager) const;

    [[nodiscard]] const SProjectDescriptor& getDescriptor() const { return descriptor; }
    [[nodiscard]] bool isPrepared() const { return prepared; }

    // Writes the current descriptor back to the .mtproj (no-op without one).
    bool saveDescriptor() const;

    // Rewrites SCRIPT_INDEX_FILE when scripts were added, changed or removed.
    // Cheap to call every frame — it compares one counter until something moves.
    void syncScriptIndex();

private:
    void ensureDirectories() const;
    void syncEngineScripting() const;
    void ensureVsCodeSettings() const;
    void ensureEditorLayout() const;

    SProjectDescriptor descriptor;
    bool               prepared = false;
    uint64_t           scriptIndexGeneration = std::numeric_limits<uint64_t>::max(); // never written yet
};

#endif // EDITOR_PROJECT_MANAGER_H
