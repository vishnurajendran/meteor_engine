// lua_scripting_engine.cpp
#include "lua_scripting_engine.h"
#include <algorithm>
#include <cctype>
#include "core/utils/meteor_paths.h"
#include "core/utils/logger.h"
#include "engine_lua_bindings.h"
#include "lua_script_instance.h"
#include "lua_script_asset.h"
#include <filesystem>

void MLuaScriptingEngineSubsystem::init()
{
    if (initialized) return;

    vm.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                      sol::lib::table, sol::lib::package);

    MLuaEngineBindings::BindEngineAPI(vm);
    loadEngineModules();

    MLOG("MLuaScriptingEngineSubsystem::Scripting engine initialized");
    initialized = true;
}

void MLuaScriptingEngineSubsystem::loadEngineModules()
{
    // Engine-owned modules are always loaded from the engine install. The
    // project keeps a synced copy only so VS Code can resolve them.
    const std::filesystem::path libDir = ENGINE_PATH(".engine_data/scripting/modules/lua").str();

    if (!std::filesystem::exists(libDir))
    {
        MLOG("No Lua module directory found, skipping module loading");
        return;
    }

    // Add lib to package.path so user scripts can require() modules
    std::string currentPath = vm["package"]["path"];
    std::string libPattern = (libDir / "?.lua").string();
    vm["package"]["path"] = currentPath + ";" + libPattern;

    // Load behaviour.lua first via script_file (not require)
    // Then populate the require cache manually so user scripts
    // calling require("behaviour") get the correct version.
    // The file ships as "Behaviour.lua". Match the name case-insensitively so
    // this also works on case-sensitive file systems (Linux/macOS).
    auto isBehaviourFile = [](const std::filesystem::path& p)
    {
        std::string name = p.filename().string();
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return name == "behaviour.lua";
    };

    std::filesystem::path behaviourPath;
    for (auto& entry : std::filesystem::directory_iterator(libDir))
        if (entry.is_regular_file() && isBehaviourFile(entry.path()))
            behaviourPath = entry.path();

    if (!behaviourPath.empty())
    {
        auto result = vm.script_file(behaviourPath.string(), sol::script_pass_on_error);
        if (!result.valid())
        {
            sol::error err = result;
            MERROR(SString::format("Failed to load behaviour.lua: {0}", err.what()));
            return;
        }
        // Cache it so require("behaviour") returns the loaded version
        vm["package"]["loaded"]["behaviour"] = vm["Behaviour"];
        MLOG("Loaded Lua module: behaviour");
    }

    // Load remaining modules the same way
    std::vector<std::filesystem::path> moduleFiles;
    for (auto& entry : std::filesystem::directory_iterator(libDir))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".lua") continue;
        // Already loaded above. The old exact-case check never matched
        // "Behaviour.lua", so the file ran twice.
        if (isBehaviourFile(entry.path())) continue;
        moduleFiles.push_back(entry.path());
    }
    std::sort(moduleFiles.begin(), moduleFiles.end());

    for (auto& path : moduleFiles)
    {
        std::string stem = path.stem().string();
        auto result = vm.script_file(path.string(), sol::script_pass_on_error);
        if (!result.valid())
        {
            sol::error err = result;
            MERROR(SString::format("Failed to load module {0}: {1}", stem, err.what()));
        }
        else
        {
            // Cache the return value for require()
            if (result.get_type() == sol::type::table)
                vm["package"]["loaded"][stem] = result;
            MLOG(SString::format("Loaded Lua module: {0}", stem));
        }
    }
}

void MLuaScriptingEngineSubsystem::tick(float deltaTime)
{
    if (!initialized) return;
}

void MLuaScriptingEngineSubsystem::cleanup()
{
    MLOG("MLuaScriptingEngineSubsystem::cleanup started");
    vm.collect_garbage();
    initialized = false;
    MLOG("MLuaScriptingEngineSubsystem::cleanup completed");
}

IScriptInstance* MLuaScriptingEngineSubsystem::createScriptInstance(IScriptAsset* asset, MSpatialEntity* owner)
{
    if (!initialized || !asset || !owner)
        return nullptr;

    // Owner is passed to constructor, stored in the instance, and
    // prepended as 'self' to every lifecycle call automatically.
    auto instance = new MLuaScriptInstance(vm, asset->getScriptSrc(), owner);

    if (!instance->isValid())
    {
        MERROR("Failed to create script instance. Script must return a class table.");
        delete instance;
        return nullptr;
    }

    MLOG("MLuaScriptingEngineSubsystem::createScriptInstance SUCCESS");
    return instance;
}

void MLuaScriptingEngineSubsystem::releaseScriptInstance(IScriptInstance* scriptInstance)
{
    delete scriptInstance;
}