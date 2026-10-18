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
        getOrLoadClass(refreshQueue[refreshCursor++], error);   // failures are logged and counted there

        const double elapsedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        if (elapsedMs >= budgetMs)
            break;
    }
    return refreshCursor < refreshQueue.size();
}

void MLuaScriptContext::loadEngineModules()
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
