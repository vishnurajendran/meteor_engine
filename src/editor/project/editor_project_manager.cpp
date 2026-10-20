//
// editor_project_manager.cpp
//

#include "editor_project_manager.h"

#include <filesystem>
#include <fstream>
#include <system_error>

#include "core/default_settings_paths.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/scripting/lua/lua_script_registry.h"
#include "core/utils/logger.h"
#include "core/utils/meteor_paths.h"
#include "editor/editor_constants.h"
#include "editor/editorassetmanager/editorassetmanager.h"

namespace fs = std::filesystem;

namespace
{
// Default VS Code settings for a project: LuaLS resolves Behaviour and the
// engine API from the project's synced .engine_data/scripting copy.
constexpr const char* DEFAULT_VSCODE_SETTINGS = R"({
    "Lua.runtime.version": "Lua 5.4",
    "Lua.runtime.path": [
        "?.lua",
        ".engine_data/scripting/modules/lua/?.lua"
    ],
    "Lua.workspace.library": [
        ".engine_data/scripting/symbols"
    ],
    "Lua.diagnostics.globals": [
        "SInput",
        "EKeyCode",
        "EForceMode",
        "MLogger",
        "MSceneManager",
        "Behaviour",
        "MMath",
        "MAssetManager",
        "MAssetHandle"
    ],
    "files.associations": {
        "*.mesl": "glsl",
        "*.skybox": "xml",
        "*.material": "xml",
        "*.scml": "xml",
        "*.mtproj": "xml"
    },
    "files.exclude": {
        "**/*.meta": true
    }
}
)";

// True if both files exist with identical content.
bool sameContent(const fs::path& a, const fs::path& b)
{
    std::error_code ec;
    if (!fs::exists(b, ec) || fs::file_size(a, ec) != fs::file_size(b, ec))
        return false;

    std::ifstream fa(a, std::ios::binary), fb(b, std::ios::binary);
    std::istreambuf_iterator<char> ia(fa), ib(fb), end;
    for (; ia != end && ib != end; ++ia, ++ib)
        if (*ia != *ib) return false;
    return ia == end && ib == end;
}

void createDir(const fs::path& p)
{
    std::error_code ec;
    fs::create_directories(p, ec);
    if (ec)
        MERROR(SString::format("ProjectManager:: cannot create {0}: {1}",
                               SString(p.generic_string()), SString(ec.message())));
}
} // namespace

// ---------------------------------------------------------------------------
// Prepare
// ---------------------------------------------------------------------------

bool MEditorProjectManager::prepare()
{
    const fs::path root = SMeteorPaths::projectRoot();

    if (SMeteorPaths::hasProjectFile())
    {
        if (!descriptor.load(SMeteorPaths::projectFile()))
            descriptor = SProjectDescriptor::makeDefault(root);
    }
    else
    {
        descriptor = SProjectDescriptor::makeDefault(root);
        MWARN("ProjectManager:: no .mtproj found; using the working directory as the project");
    }

    MLOG(SString::format("ProjectManager:: opening project '{0}' at {1}",
                         descriptor.name, SString(root.generic_string())));

    ensureDirectories();
    syncEngineScripting();
    ensureVsCodeSettings();
    ensureEditorLayout();

    prepared = true;
    return true;
}

void MEditorProjectManager::ensureDirectories() const
{
    for (const auto& folder : descriptor.assetFolders)
        createDir(PROJECT_PATH(folder).str());

    // Settings are auto-generated on first save; the folders just need to exist.
    createDir(PROJECT_PATH(DEFAULT_SETTINGS_PATH).str());
    createDir(PROJECT_PATH(SEditorPaths::DIR_THUMBNAILS_CACHE).str());
    createDir(PROJECT_PATH(SEditorPaths::DIR_TEMP).str());
    createDir(PROJECT_PATH(".vscode").str());
}

void MEditorProjectManager::syncEngineScripting() const
{
    // Running straight from the build output: nothing to copy.
    if (SMeteorPaths::isEngineProject())
        return;

    const fs::path src = ENGINE_PATH(ENGINE_SCRIPTING_DIR).str();
    const fs::path dst = PROJECT_PATH(ENGINE_SCRIPTING_DIR).str();

    // The script index is per project — never copy the engine folder's one
    // (it exists there when the engine itself was opened as a project).
    const fs::path scriptIndex = fs::path(SCRIPT_INDEX_FILE).lexically_relative(ENGINE_SCRIPTING_DIR);

    std::error_code ec;
    if (!fs::exists(src, ec))
    {
        MWARN(SString("ProjectManager:: engine scripting data not found at ") + SString(src.generic_string()));
        return;
    }

    int updated = 0;
    for (const auto& entry : fs::recursive_directory_iterator(src, ec))
    {
        if (!entry.is_regular_file(ec)) continue;

        const fs::path rel    = fs::relative(entry.path(), src, ec);
        if (rel == scriptIndex) continue;
        const fs::path target = dst / rel;
        if (sameContent(entry.path(), target)) continue;

        createDir(target.parent_path());
        fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
        if (ec)
            MERROR(SString::format("ProjectManager:: cannot copy {0}: {1}",
                                   SString(rel.generic_string()), SString(ec.message())));
        else
            ++updated;
    }

    if (updated > 0)
        MLOG(SString::format("ProjectManager:: updated {0} engine scripting file(s) in the project",
                             SString::fromInt(updated)));
}

void MEditorProjectManager::ensureVsCodeSettings() const
{
    const fs::path settings = PROJECT_PATH(".vscode/settings.json").str();
    std::error_code ec;
    if (fs::exists(settings, ec))
        return;   // never overwrite the user's settings

    std::ofstream out(settings, std::ios::binary);
    out << DEFAULT_VSCODE_SETTINGS;
    MLOG("ProjectManager:: created .vscode/settings.json");
}

void MEditorProjectManager::ensureEditorLayout() const
{
    // ImGui keeps the window/docking layout in imgui.ini in the working
    // directory (the project). Seed a new project with the engine's default
    // layout so it doesn't open with every window floating. Never overwrite:
    // after this the file belongs to the project.
    if (SMeteorPaths::isEngineProject())
        return;

    const fs::path target = PROJECT_PATH("imgui.ini").str();
    const fs::path source = ENGINE_PATH("imgui.ini").str();

    std::error_code ec;
    if (fs::exists(target, ec) || !fs::exists(source, ec))
        return;

    fs::copy_file(source, target, ec);
    if (!ec)
        MLOG("ProjectManager:: created imgui.ini from the engine's default layout");
}

// ---------------------------------------------------------------------------
// Asset manager
// ---------------------------------------------------------------------------

void MEditorProjectManager::applyToAssetManager(MEditorAssetManager& assetManager) const
{
    assetManager.setAssetSource(MAssetManager::createProjectSource(descriptor.assetFolders));

    auto& templates = assetManager.getTemplateRegistry();
    templates.setGlobalToken("__PROJECT_NAME__", descriptor.name);

    // Project templates are searched after the engine's, so a project can
    // override a template file by name or a template id in its templates.xml.
    if (!SMeteorPaths::isEngineProject())
        templates.addTemplateDirectory(PROJECT_PATH(PROJECT_TEMPLATES_DIR));
}

bool MEditorProjectManager::saveDescriptor() const
{
    if (!SMeteorPaths::hasProjectFile())
        return false;
    return descriptor.save(SMeteorPaths::projectFile());
}

// ---------------------------------------------------------------------------
// Script index (VS Code / LuaLS)
// ---------------------------------------------------------------------------

void MEditorProjectManager::syncScriptIndex()
{
    const uint64_t generation = MLuaScriptRegistry::getGeneration();
    if (generation == scriptIndexGeneration)
        return;
    scriptIndexGeneration = generation;

    // Built by the registry from each script's parsed MODULE / CLASS.
    const std::string text = MLuaScriptRegistry::buildTypeIndex();

    const fs::path target = PROJECT_PATH(SCRIPT_INDEX_FILE).str();

    // Only touch the file when it changes — every write makes LuaLS re-index.
    {
        std::ifstream in(target, std::ios::binary);
        if (in)
        {
            const std::string existing((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            if (existing == text)
                return;
        }
    }

    createDir(target.parent_path());
    std::ofstream out(target, std::ios::binary | std::ios::trunc);
    out << text;
    if (!out)
        MERROR(SString::format("ProjectManager:: cannot write {0}", SString(target.generic_string())));
}
