// lua_scripting_engine.cpp
#include "lua_scripting_engine.h"

#include "core/engine/domain/engine_domain_subsystem.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/logger.h"
#include "lua_script_asset.h"
#include "lua_script_context.h"
#include "lua_script_instance.h"

void MLuaScriptingEngineSubsystem::init()
{
    // Lua state lives in each domain's script context, created by
    // MEngineDomainSubsystem.
    MLOG("MLuaScriptingEngineSubsystem::Scripting engine initialized");
}

void MLuaScriptingEngineSubsystem::tick(float deltaTime)
{
}

void MLuaScriptingEngineSubsystem::cleanup()
{
    MLOG("MLuaScriptingEngineSubsystem::cleanup completed");
}

IScriptInstance* MLuaScriptingEngineSubsystem::createScriptInstance(IScriptAsset* asset, MSpatialEntity* owner)
{
    if (!asset || !owner)
        return nullptr;

    auto* luaAsset = dynamic_cast<MLuaScriptAsset*>(asset);
    if (!luaAsset)
    {
        MERROR("MLuaScriptingEngineSubsystem::createScriptInstance: asset is not a Lua script");
        return nullptr;
    }

    auto* domains = MEngineSubsystemRegistry::getSubsystem<MEngineDomainSubsystem>();
    auto* domain  = domains ? domains->getCurrentActiveDomain() : nullptr;
    if (!domain)
    {
        MERROR("MLuaScriptingEngineSubsystem::createScriptInstance: no active engine domain");
        return nullptr;
    }

    return domain->getScriptContext().createInstance(luaAsset, owner);
}

void MLuaScriptingEngineSubsystem::releaseScriptInstance(IScriptInstance* scriptInstance)
{
    auto* instance = dynamic_cast<MLuaScriptInstance*>(scriptInstance);
    if (!instance)
    {
        delete scriptInstance;
        return;
    }

    // Back to the context that created it. A detached instance (its domain
    // was removed first) has no context left and is simply deleted.
    if (auto* context = instance->getContext())
        context->releaseInstance(instance);
    else
        delete instance;
}
