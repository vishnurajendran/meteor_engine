// lua_script_context.cpp
#include "lua_script_context.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include "core/utils/logger.h"
#include "core/utils/meteor_paths.h"
#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "engine_lua_bindings.h"
#include "lua_script_asset.h"
#include "lua_script_instance.h"
#include "lua_script_registry.h"

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

MLuaScriptContext::MLuaScriptContext(const SString& debugName) : debugName(debugName)
{
    vm.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                      sol::lib::table, sol::lib::package);

    MLuaEngineBindings::BindEngineAPI(vm);
    loadEngineModules();
    registerImport();

    MLOG(SString::format("MLuaScriptContext:: '{0}' ready", debugName));
}

MLuaScriptContext::~MLuaScriptContext()
{
    // Instances still alive (an entity outliving its domain) are cut loose
    // while the VM still exists - releasing them later must not touch a
    // closed lua_State.
    if (!liveInstances.empty())
        MWARN(SString::format("MLuaScriptContext:: '{0}' closing with {1} live script instance(s); detaching them",
                              debugName, SString::fromInt(static_cast<int>(liveInstances.size()))));
    for (auto* instance : liveInstances)
        instance->detach();
    liveInstances.clear();

    // Drop our references first so the classes can actually be collected.
    classCache.clear();
    vm.collect_garbage();
    MLOG(SString::format("MLuaScriptContext:: '{0}' closed", debugName));
}

// ---------------------------------------------------------------------------
// Instances
// ---------------------------------------------------------------------------

IScriptInstance* MLuaScriptContext::createInstance(MLuaScriptAsset* asset, MSpatialEntity* owner)
{
    if (!asset || !owner)
        return nullptr;

    SString error;
    const SLuaClass* scriptClass = getOrLoadClass(asset, error);
    if (!scriptClass)
    {
        MERROR(SString::format("Script '{0}' was not started on '{1}': {2}",
                               asset->getPath(), owner->getName(), error));
        return nullptr;
    }

    // The instance is { __entity = owner } with the class as its metatable,
    // and is passed as 'self' to every lifecycle call.
    auto* instance = new MLuaScriptInstance(vm, scriptClass->cls, scriptClass->env, owner, this);
    liveInstances.insert(instance);
    return instance;
}

void MLuaScriptContext::releaseInstance(MLuaScriptInstance* instance)
{
    if (!instance)
        return;
    liveInstances.erase(instance);
    delete instance;
}

// ---------------------------------------------------------------------------
// Script refresh
// ---------------------------------------------------------------------------

bool MLuaScriptContext::isRefreshing() const
{
    return MLuaScriptRegistry::getGeneration() != refreshGeneration || refreshCursor < refreshQueue.size();
}

bool MLuaScriptContext::refreshStep(double budgetMs)
{
    // Any script added, changed or removed since the last refresh restarts it.
    // syncClassCache() drops every class and resets the error count for the
    // new generation; this then compiles every script against it.
    const uint64_t generation = MLuaScriptRegistry::getGeneration();
    if (generation != refreshGeneration)
    {
        syncClassCache();
        refreshQueue      = MLuaScriptRegistry::getAllScripts();
        refreshCursor     = 0;
        refreshGeneration = generation;
    }

    // Compile until the frame budget runs out. At least one script per call,
    // so a single slow script still makes progress.
    const auto start = std::chrono::steady_clock::now();
    while (refreshCursor < refreshQueue.size())
    {
        SString error;
        auto* asset = refreshQueue[refreshCursor++];
        const SLuaClass* scriptClass = getOrLoadClass(asset, error);   // failures are logged and counted there
        publishPublicFields(asset, scriptClass);

        const double elapsedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        if (elapsedMs >= budgetMs)
            break;
    }
    return refreshCursor < refreshQueue.size();
}

void MLuaScriptContext::loadEngineModules()
{
    const auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (assetManager && assetManager->providesCoreAssets())
    {
        loadEngineModulesFromCoreAssets(*assetManager);
        return;
    }

    // Engine-owned modules are loaded from the engine install. The project
    // keeps a synced copy only so VS Code can resolve them.
    const std::filesystem::path libDir = ENGINE_PATH(".engine_data/scripting/modules/lua").str();

    if (!std::filesystem::exists(libDir))
    {
        MLOG("No Lua module directory found, skipping module loading");
        return;
    }

    // Put the engine module folder FIRST on package.path. The default path
    // contains "./?.lua", and the working directory is the project root, so
    // appending let a project file with the same name shadow an engine module.
    std::string currentPath = vm["package"]["path"];
    std::string libPattern = (libDir / "?.lua").string();
    vm["package"]["path"] = libPattern + ";" + currentPath;

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
        // Cache it so require() returns the loaded version. Register the
        // file's own name too: require("Behaviour") missed the lowercase key,
        // found the file through package.path (case-insensitive on Windows)
        // and ran it again, giving scripts a second, different Behaviour.
        vm["package"]["loaded"]["behaviour"] = vm["Behaviour"];
        vm["package"]["loaded"][behaviourPath.stem().string()] = vm["Behaviour"];
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

void MLuaScriptContext::loadEngineModulesFromCoreAssets(const IAssetManagerSubsystem& assetManager)
{
    struct SModule
    {
        SString stem;     // module name, e.g. "Behaviour"
        SString source;
        SString chunk;    // shown in Lua error messages
    };
    std::vector<SModule> modules;

    for (const SString& name : assetManager.listCoreAssets(IAssetManagerSubsystem::CORE_LUA_MODULES))
    {
        if (!name.endsWith(".lua"))
            continue;

        SString source;
        if (!assetManager.loadCoreAsset(name, source))
        {
            MERROR(SString("Failed to read Lua module ") + name);
            continue;
        }
        const SString file = name.substring(name.findLast('/') + 1);
        modules.push_back({ file.substring(0, file.size() - 4), source, "@" + file });
    }

    if (modules.empty())
    {
        MLOG("No Lua modules in the package, skipping module loading");
        return;
    }

    // There is no engine folder on package.path here, so let require() find
    // any engine module by name - also one that has not run yet.
    for (const auto& m : modules)
    {
        const SString source = m.source;
        const SString chunk  = m.chunk;
        vm["package"]["preload"][m.stem.str()] = [this, source, chunk](sol::this_state) -> sol::object
        {
            auto result = vm.script(source.str(), sol::script_pass_on_error, chunk.str());
            if (!result.valid())
            {
                sol::error err = result;
                MERROR(SString::format("Failed to load module {0}: {1}", chunk, SString(err.what())));
                return sol::make_object(vm, sol::lua_nil);
            }
            return result.get<sol::object>();
        };
    }

    // Behaviour first, as from disk. Cached under both spellings so
    // require("behaviour") and require("Behaviour") return the same class.
    for (const auto& m : modules)
    {
        if (m.stem.toLower() != "behaviour")
            continue;

        auto result = vm.script(m.source.str(), sol::script_pass_on_error, m.chunk.str());
        if (!result.valid())
        {
            sol::error err = result;
            MERROR(SString::format("Failed to load behaviour.lua: {0}", err.what()));
            return;
        }
        vm["package"]["loaded"]["behaviour"] = vm["Behaviour"];
        vm["package"]["loaded"][m.stem.str()] = vm["Behaviour"];
        MLOG("Loaded Lua module: behaviour");
    }

    // The rest in name order, unless a module already require()d it.
    std::sort(modules.begin(), modules.end(), [](const SModule& a, const SModule& b) { return a.stem < b.stem; });
    for (const auto& m : modules)
    {
        if (m.stem.toLower() == "behaviour")
            continue;
        sol::object loaded = vm["package"]["loaded"][m.stem.str()];
        if (loaded.valid() && loaded.get_type() != sol::type::lua_nil)
            continue;

        auto result = vm.script(m.source.str(), sol::script_pass_on_error, m.chunk.str());
        if (!result.valid())
        {
            sol::error err = result;
            MERROR(SString::format("Failed to load module {0}: {1}", m.stem, err.what()));
            continue;
        }
        if (result.get_type() == sol::type::table)
            vm["package"]["loaded"][m.stem.str()] = result;
        MLOG(SString::format("Loaded Lua module: {0}", m.stem));
    }
}

// ---------------------------------------------------------------------------
// Class cache
// ---------------------------------------------------------------------------

SString MLuaScriptContext::displayName(MLuaScriptAsset* asset)
{
    const auto& declaration = asset->getDeclaration();
    return declaration.isImportable() ? declaration.fullName : asset->getPath();
}

void MLuaScriptContext::syncClassCache()
{
    const uint64_t generation = MLuaScriptRegistry::getGeneration();
    if (generation == classCacheGeneration)
        return;

    // Never flush in the middle of a compile - the classes on the loading
    // stack are referenced further up the call chain. Scripts only change
    // between frames, so this is purely defensive.
    if (!loadingStack.empty())
        return;

    // Instances that already exist keep a reference to their old class, so
    // they keep working until the next Play reloads the scene.
    classCache.clear();
    errorCount = 0;
    classCacheGeneration = generation;
}

const MLuaScriptContext::SLuaClass* MLuaScriptContext::getOrLoadClass(
    MLuaScriptAsset* asset, SString& outError)
{
    syncClassCache();

    const SString path = asset->getPath();
    // A failed compile is logged once, in full, when it happens - everything
    // that depends on the script just points back at that error.
    const auto failedMessage = [&] {
        return SString::format("failed to load - see the error logged for '{0}'", displayName(asset));
    };

    if (const auto it = classCache.find(path); it != classCache.end())
    {
        if (!it->second.error.empty())
        {
            outError = failedMessage();
            return nullptr;
        }
        return &it->second;
    }

    // An import cycle shows up as the script already being mid-compile.
    if (std::ranges::find(loadingStack, asset) != loadingStack.end())
    {
        std::string chain;
        bool inCycle = false;
        for (auto* loading : loadingStack)
        {
            inCycle = inCycle || loading == asset;
            if (inCycle)
                chain += displayName(loading).str() + " -> ";
        }
        chain += displayName(asset).str();
        // Not cached: the cycle belongs to this call chain, not the script.
        outError = SString("import cycle: " + chain + " (import inside a function to break it)");
        return nullptr;
    }

    loadingStack.push_back(asset);
    SLuaClass compiled = compileClass(asset);   // may re-enter through import()
    loadingStack.pop_back();

    if (!compiled.error.empty())
    {
        ++errorCount;
        MERROR(SString::format("Script '{0}' failed to load: {1}", displayName(asset), compiled.error));
    }

    auto& entry = classCache[path] = std::move(compiled);
    if (!entry.error.empty())
    {
        outError = failedMessage();
        return nullptr;
    }
    return &entry;
}

namespace
{
// What MODULE / CLASS recorded while one script ran. Checked against
// the parsed declaration afterwards, so the name import() resolves by is the
// name the script really declared.
struct SRuntimeDeclaration
{
    std::string nameSpace = MLuaScriptRegistry::DEFAULT_NAMESPACE;
    std::string className;
    bool        moduleCalled  = false;
    bool        classDeclared = false;
    sol::table  cls;          // set when the member table was supplied
};

// Gives one script environment its own MODULE and CLASS. They are
// per-script rather than global because a script can import() another one
// between its MODULE and CLASS lines - a shared global would pick up
// the imported script's MODULE.
void bindDeclarationFunctions(sol::environment& env, const std::shared_ptr<SRuntimeDeclaration>& declared)
{
    env.set_function("MODULE", [declared](const std::string& nameSpace)
    {
        if (declared->classDeclared)
            throw std::runtime_error("MODULE must come before CLASS");
        if (declared->moduleCalled)
            throw std::runtime_error("MODULE can only be used once per script");
        declared->moduleCalled = true;
        declared->nameSpace    = nameSpace;
    });

    // CLASS("Name", Base) returns a function taking the member table,
    // so the call reads CLASS("Name", Base) { ... }.
    env.set_function("CLASS", [declared](sol::this_state ts, const std::string& className,
                                                sol::object base) -> sol::object
    {
        if (declared->classDeclared)
            throw std::runtime_error("CLASS can only be used once per script");

        sol::protected_function extend;
        if (base.get_type() == sol::type::table)
            extend = base.as<sol::table>()["extend"];
        if (!extend.valid())
            throw std::runtime_error("CLASS(\"" + className + "\", base): base must be Behaviour or a script class");

        declared->classDeclared = true;
        declared->className     = className;

        sol::table baseClass = base.as<sol::table>();
        return sol::make_object(sol::state_view(ts), [declared, baseClass, extend](sol::object members) -> sol::table
        {
            if (declared->cls.valid())
                throw std::runtime_error("CLASS: the member table was already given");

            auto created = members.get_type() == sol::type::table ? extend(baseClass, members) : extend(baseClass);
            if (!created.valid())
            {
                sol::error err = created;
                throw std::runtime_error(err.what());
            }

            sol::table cls = created;
            cls.raw_set("__className", declared->className);
            declared->cls = cls;
            return cls;
        });
    });
}
} // namespace

MLuaScriptContext::SLuaClass MLuaScriptContext::compileClass(MLuaScriptAsset* asset)
{
    SLuaClass result;
    result.env = sol::environment(vm, sol::create, vm.globals());

    auto declared = std::make_shared<SRuntimeDeclaration>();
    bindDeclarationFunctions(result.env, declared);

    // A chunk name starting with '@' makes Lua report errors as path:line.
    const std::string chunkName = "@" + asset->getPath().str();
    auto loaded = vm.script(asset->getScriptSrc().str(), result.env, sol::script_pass_on_error, chunkName);

    // Declaring only happens while the file loads - methods calling these
    // later get a plain "attempt to call a nil value". Clearing them also
    // drops `declared`'s reference to the class: class methods keep the
    // environment alive, so a reference back through these closures would
    // be a cycle Lua's collector can't see through.
    result.env["MODULE"]       = sol::lua_nil;
    result.env["CLASS"] = sol::lua_nil;
    sol::table declaredClass = declared->cls;
    declared->cls = sol::table();

    if (!loaded.valid())
    {
        sol::error err = loaded;
        result.error = err.what();
        return result;
    }

    const auto& declaration = asset->getDeclaration();

    if (declared->classDeclared && !declaredClass.valid())
    {
        // Plain concatenation: SString::format would read "{ ... }" as a placeholder.
        result.error = SString("CLASS(\"" + declared->className + "\", ...) needs its member table: CLASS(\""
                               + declared->className + "\", Base) { ... }");
        return result;
    }

    if (loaded.get_type() != sol::type::table)
    {
        result.error = "the script must end with 'return ClassName'";
        return result;
    }

    sol::table cls = loaded;

    if (declared->classDeclared)
    {
        if (cls.pointer() != declaredClass.pointer())
        {
            result.error = "the script must return the class created by CLASS";
            return result;
        }

        // The parse and the run must agree, or import() would resolve a name
        // the script never really declared.
        const std::string ranAs = declared->nameSpace + "." + declared->className;
        if (!declaration.isImportable() || declaration.fullName.str() != ranAs)
        {
            result.error = SString::format("the script declared '{0}' when it ran, but its source reads as '{1}' - "
                                           "keep MODULE and CLASS at the top level with literal names",
                                           SString(ranAs),
                                           declaration.isImportable() ? declaration.fullName : SString("no class"));
            return result;
        }
    }

    else if (declaration.isImportable())
    {
        // The source has a CLASS, but it never ran (e.g. inside an if).
        result.error = SString::format("the source declares '{0}', but CLASS never ran - "
                                       "keep it at the top level of the script", declaration.fullName);
        return result;
    }

    // Behaviour:extend sets __index on the class; without it an instance
    // could not find any method.
    if (cls.raw_get<sol::object>("__index").get_type() != sol::type::table)
    {
        result.error = "the returned table is not a class - declare it with CLASS(\"Name\", Behaviour) { ... }";
        return result;
    }

    // Identification for debugging (print(getmetatable(obj).__fullName)).
    cls.raw_set("__scriptPath", asset->getPath().str());
    if (declaration.isImportable())
        cls.raw_set("__fullName", declaration.fullName.str());

    result.cls = cls;
    return result;
}

// ---------------------------------------------------------------------------
// import()
// ---------------------------------------------------------------------------

void MLuaScriptContext::registerImport()
{
    // Errors are thrown as C++ exceptions: sol2 turns them into Lua errors at
    // the call boundary. luaL_error would longjmp straight over the C++
    // destructors in this function.
    vm.set_function("import", [this](sol::this_state ts, const std::string& fullName) -> sol::table
    {
        lua_State* L = ts;
        // "path:line: " of the import() call, so the error points at it.
        luaL_where(L, 1);
        const std::string where = lua_tostring(L, -1);
        lua_pop(L, 1);

        const auto scripts = MLuaScriptRegistry::find(fullName);
        if (scripts.empty())
            throw std::runtime_error(where + "import: no script declares '" + fullName + "'");

        if (scripts.size() > 1)
        {
            std::string paths;
            for (const auto* script : scripts)
                paths += "\n    " + script->getPath().str();
            throw std::runtime_error(where + "import: '" + fullName
                                     + "' is declared by more than one script:" + paths);
        }

        SString error;
        const SLuaClass* scriptClass = getOrLoadClass(scripts.front(), error);
        if (!scriptClass)
            throw std::runtime_error(where + "import: cannot load '" + fullName + "': " + error.str());

        return scriptClass->cls;
    });
}

// ---------------------------------------------------------------------------
// Public fields (behaviour tuning)
// ---------------------------------------------------------------------------

namespace
{
// A public field's value as an SDynValue, or false for anything that is not
// a boolean, number or string. 5 stays an integer, 5.0 a float (Lua 5.4
// keeps the two apart).
bool toFieldValue(const sol::object& value, SDynValue& out)
{
    switch (value.get_type())
    {
        case sol::type::boolean:
            out.setBool(value.as<bool>());
            return true;
        case sol::type::string:
            out.setString(value.as<std::string>());
            return true;
        case sol::type::number:
        {
            lua_State* L = value.lua_state();
            value.push();
            const bool isInteger = lua_isinteger(L, -1);
            lua_pop(L, 1);
            if (isInteger)
                out.setInt(static_cast<int>(value.as<lua_Integer>()));
            else
                out.setFloat(value.as<float>());
            return true;
        }
        default:
            return false;
    }
}
} // namespace

void MLuaScriptContext::publishPublicFields(MLuaScriptAsset* asset, const SLuaClass* scriptClass)
{
    SLuaScriptFieldSet fieldSet;
    if (!scriptClass)
    {
        MLuaScriptRegistry::setPublicFields(asset, std::move(fieldSet));   // compiled = false
        return;
    }
    fieldSet.compiled = true;

    // The class chain, derived first: CameraController -> ... up to (not
    // including) Behaviour. Every compiled script class carries __scriptPath;
    // Behaviour does not, which ends the walk.
    std::vector<sol::table> chain;
    sol::object level = scriptClass->cls;
    while (level.get_type() == sol::type::table)
    {
        sol::table cls = level;
        if (cls.raw_get<sol::object>("__scriptPath").get_type() != sol::type::string)
            break;
        chain.push_back(cls);

        sol::object meta = cls[sol::metatable_key];
        if (meta.get_type() != sol::type::table)
            break;
        level = meta.as<sol::table>().raw_get<sol::object>("__index");
    }

    // Base first, so inherited fields come before the class's own and a
    // subclass's default replaces its parent's.
    std::unordered_map<std::string, size_t> indexByName;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
    {
        // Source order for this level comes from its own script's parsed
        // member list; keys the parser didn't see go last, alphabetically.
        std::unordered_map<std::string, size_t> sourceOrder;
        if (const auto* levelAsset = MLuaScriptRegistry::findByPath(it->raw_get<std::string>("__scriptPath")))
        {
            const auto& members = levelAsset->getDeclaration().members;
            for (size_t i = 0; i < members.size(); ++i)
                sourceOrder.emplace(members[i].name.str(), i);
        }

        std::vector<std::pair<std::string, SDynValue>> found;
        for (const auto& [key, value] : *it)   // raw iteration - this level's own keys only
        {
            if (key.get_type() != sol::type::string)
                continue;
            std::string name = key.as<std::string>();
            if (name.empty() || name[0] == '_')   // '_' = private, also skips __index / __className ...
                continue;
            SDynValue fieldValue;
            if (toFieldValue(value, fieldValue))
                found.emplace_back(std::move(name), std::move(fieldValue));
        }

        std::ranges::sort(found, [&](const auto& a, const auto& b)
        {
            const auto ia = sourceOrder.find(a.first);
            const auto ib = sourceOrder.find(b.first);
            if (ia != sourceOrder.end() && ib != sourceOrder.end()) return ia->second < ib->second;
            if (ia != sourceOrder.end()) return true;
            if (ib != sourceOrder.end()) return false;
            return a.first < b.first;
        });

        for (auto& [name, value] : found)
        {
            if (const auto existing = indexByName.find(name); existing != indexByName.end())
            {
                fieldSet.fields[existing->second].defaultValue = value;   // subclass default wins
                continue;
            }
            indexByName.emplace(name, fieldSet.fields.size());
            fieldSet.fields.push_back({SString(name), value});
        }
    }

    MLuaScriptRegistry::setPublicFields(asset, std::move(fieldSet));
}
