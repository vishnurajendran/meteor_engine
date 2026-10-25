//
// lua_script_context.h
//
// One self-contained Lua world: its own VM, engine bindings, Behaviour
// module, import() and compiled-class cache. Each engine domain owns one
// (see MEngineDomain), so edit-time and play-time scripts never share state:
//
//   DOMAIN_0 (editor)  - the sandbox. Compiles every script on each change
//                        (the script refresh) to report errors; never holds
//                        entity instances in the editor.
//   DOMAIN_1 (runtime) - created on Play, destroyed on Stop. Entity
//                        instances live here; it starts from a clean VM.
//
// In the player, DOMAIN_0 is the game and is the only context.
//
// Classes vs instances: each script file is compiled once into a class
// table, cached by asset path. import() and entity instances both go
// through the cache, so every script in this context sees the same class
// table - which is what makes casts like CameraController(x) reliable.
// Any script being added, changed or removed flushes the whole cache (the
// registry generation moves).
//

#ifndef LUA_SCRIPT_CONTEXT_H
#define LUA_SCRIPT_CONTEXT_H

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/utils/sstring.h"
#include "sol/sol.hpp"

class IAssetManagerSubsystem;
class IScriptInstance;
class MLuaScriptAsset;
class MLuaScriptInstance;
class MSpatialEntity;

class MLuaScriptContext
{
public:
    explicit MLuaScriptContext(const SString& debugName);
    ~MLuaScriptContext();

    MLuaScriptContext(const MLuaScriptContext&)            = delete;
    MLuaScriptContext& operator=(const MLuaScriptContext&) = delete;

    // ---- instances ---------------------------------------------------------
    // Compiles the script's class if needed and returns an instance for the
    // entity, or nullptr (with the error logged).
    IScriptInstance* createInstance(MLuaScriptAsset* asset, MSpatialEntity* owner);
    void releaseInstance(MLuaScriptInstance* instance);

    // ---- script refresh ----------------------------------------------------
    // Compiles every script for the current registry generation, spread over
    // calls: does work for about budgetMs, returns true while work remains.
    // Any script change restarts it.
    bool refreshStep(double budgetMs);
    [[nodiscard]] bool isRefreshing() const;
    [[nodiscard]] uint32_t getRefreshDone()  const { return static_cast<uint32_t>(refreshCursor); }
    [[nodiscard]] uint32_t getRefreshTotal() const { return static_cast<uint32_t>(refreshQueue.size()); }

    // Scripts that failed to compile in the current generation. Play is
    // blocked while the editor's context reports more than 0.
    [[nodiscard]] uint32_t getErrorCount() const { return errorCount; }

    [[nodiscard]] const SString& getDebugName() const { return debugName; }

private:
    struct SLuaClass
    {
        sol::environment env;   // the script's globals sandbox, shared by its instances
        sol::table       cls;   // the table the script returned
        SString          error; // set when compiling failed - failures are cached too
    };

    // Sets up package.path and loads the engine Lua modules (Behaviour
    // first, then everything else alphabetically).
    void loadEngineModules();
    // Published game: the modules are core assets, served by the asset manager.
    void loadEngineModulesFromCoreAssets(const IAssetManagerSubsystem& assetManager);

    // Registers the global import(fullName) function.
    void registerImport();

    // Returns the compiled class for the script, compiling it on first use.
    // Returns nullptr and fills outError on failure. The pointer stays valid
    // until the cache is flushed (unordered_map keeps element addresses).
    const SLuaClass* getOrLoadClass(MLuaScriptAsset* asset, SString& outError);
    SLuaClass compileClass(MLuaScriptAsset* asset);

    // Drops every cached class if any script changed since the last call.
    void syncClassCache();

    // Reads the compiled class's public fields (including inherited ones)
    // and publishes them to the registry for the inspector. A null class
    // publishes "failed to compile".
    void publishPublicFields(MLuaScriptAsset* asset, const SLuaClass* scriptClass);

    // full name if the script has one, otherwise its path - for messages
    static SString displayName(MLuaScriptAsset* asset);

    SString debugName;

    // Declared first so it is destroyed last: everything below holds
    // references into it.
    sol::state vm;

    std::unordered_map<SString, SLuaClass> classCache;   // keyed by asset path
    uint64_t classCacheGeneration = 0;
    uint32_t errorCount = 0;                             // failed entries in classCache
    std::vector<MLuaScriptAsset*> loadingStack;          // scripts mid-compile - detects import cycles

    std::unordered_set<MLuaScriptInstance*> liveInstances;

    std::vector<MLuaScriptAsset*> refreshQueue;
    size_t   refreshCursor     = 0;
    uint64_t refreshGeneration = 0;
};

#endif // LUA_SCRIPT_CONTEXT_H
