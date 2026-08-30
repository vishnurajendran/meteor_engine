// lua_scripting_engine.h
#ifndef SCRIPTING_ENGINE_H
#define SCRIPTING_ENGINE_H
#include "core/object/object.h"
#include "core/engine/scripting/interface/scripting_engine_interface.h"
#include "sol/sol.hpp"

class IScriptAsset;
class MObject;

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

    sol::state vm;
    bool initialized = false;
};

#endif //SCRIPTING_ENGINE_H