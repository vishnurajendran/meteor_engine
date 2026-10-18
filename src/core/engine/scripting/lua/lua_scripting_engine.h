// lua_scripting_engine.h
#ifndef SCRIPTING_ENGINE_H
#define SCRIPTING_ENGINE_H
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "core/object/object.h"
#include "core/engine/scripting/interface/scripting_engine_interface.h"
#include "sol/sol.hpp"

class IScriptAsset;
class MLuaScriptAsset;
class MObject;

//
// Scripts are split into CLASSES and INSTANCES:
//   - each script file is compiled once into a class table, cached by asset
//     path. import() and entity instances both go through this cache, so
//     every script sees the same class table — which is what makes casts
//     like CameraController(entt:getScript()) reliable.
//   - each entity gets a small instance table whose metatable is the class
//     (see MLuaScriptInstance).
//
// Any script being added, changed or removed flushes the whole cache (the
// registry generation moves) — a script holding an imported class in a
// local would otherwise keep a stale copy and its casts would silently fail.
//

class MLuaScriptingEngineSubsystem : public MObject, public IScriptingEngineSubsystem
{
    DEFINE_OBJECT_SUBCLASS(MLuaScriptingEngineSubsystem)
public:
    MLuaScriptingEngineSubsystem() = default;
    virtual ~MLuaScriptingEngineSubsystem() override = default;

    void init() override;
    void tick(float deltaTime) override;
    void cleanup() override;

    IScriptInstance* createScriptInstance(IScriptAsset* asset, MSpatialEntity* owner) override;
    void releaseScriptInstance(IScriptInstance* scriptInstance) override;

private:
    /// Sets up package.path and uses require() to load engine Lua
    /// modules from .engine_settings/lib/. Behaviour loads first,
    /// then everything else alphabetically.
    void loadEngineModules();

    // Registers the global import(fullName) function.
    void registerImport();

    struct SLuaClass
    {
        sol::environment env;   // the script's globals sandbox, shared by its instances
        sol::table       cls;   // the table the script returned
        SString          error; // set when compiling failed — failures are cached too
    };

    // Returns the compiled class for the script, compiling it on first use.
    // Returns nullptr and fills outError on failure. The pointer stays valid
    // until the cache is flushed (unordered_map keeps element addresses).
    const SLuaClass* getOrLoadClass(MLuaScriptAsset* asset, SString& outError);
    SLuaClass compileClass(MLuaScriptAsset* asset);

    // Drops every cached class if any script changed since the last call.
    void syncClassCache();

    // full name if the script has one, otherwise its path — for messages
    static SString displayName(MLuaScriptAsset* asset);

    sol::state vm;
    bool initialized = false;

    std::unordered_map<SString, SLuaClass> classCache;   // keyed by asset path
    uint64_t classCacheGeneration = 0;
    std::vector<MLuaScriptAsset*> loadingStack;          // scripts mid-compile — detects import cycles
};

#endif //SCRIPTING_ENGINE_H