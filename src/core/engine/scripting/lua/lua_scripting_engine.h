// lua_scripting_engine.h
#ifndef SCRIPTING_ENGINE_H
#define SCRIPTING_ENGINE_H
#include "core/object/object.h"
#include "core/engine/scripting/interface/scripting_engine_interface.h"

class IScriptAsset;
class MObject;

//
// The scripting subsystem's front door. It owns no Lua state itself: each
// engine domain has its own MLuaScriptContext (see lua_script_context.h), and
// new instances are created in the current (top) domain's context.
// Releasing goes back to the context that created the instance, whichever
// domain is on top by then.
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
};

#endif //SCRIPTING_ENGINE_H
