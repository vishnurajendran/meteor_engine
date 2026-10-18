// lua_script_instance.h
#ifndef LUA_SCRIPT_INSTANCE_H
#define LUA_SCRIPT_INSTANCE_H

#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scripting/interface/script_instance.h"
#include "core/object/object.h"
#include "core/object/object_class_macros.h"
#include "sol/sol.hpp"

class MLuaScriptContext;

class MLuaScriptInstance : public MObject, public IScriptInstance
{
    DEFINE_OBJECT_SUBCLASS(MLuaScriptInstance)
public:
    // One entity's instance of a compiled script class: a table
    // { __entity = owner } whose metatable is the class. Fields written
    // through self land on the instance; methods and defaults come from
    // the class, which is shared by every entity using the script.
    MLuaScriptInstance(sol::state& vm, const sol::table& scriptClass, const sol::environment& classEnv,
                       MSpatialEntity* owner, MLuaScriptContext* context);
    ~MLuaScriptInstance() override = default;

    bool isValid() const override { return valid; }

    bool callFunc(const SString& funcName, const std::unordered_map<SString, SDynValue>& params, SDynValue& result) override;
    bool callFunc(const SString& funcName, const std::vector<SDynValue>& args, SDynValue& result) override;

    void setEnvVar(const SString& name, const SDynValue& value) override;

    bool setField(const SString& name, const SDynValue& value) override;
    void clearField(const SString& name) override;

    template<typename T>
    void setEnvVarTyped(const SString& name, T value)
    {
        if (valid)
            env[name.c_str()] = value;
    }

    // The script's globals sandbox. Shared by every instance of the class,
    // so setEnvVar() is visible to all entities using the same script.
    sol::environment& getEnvironment() { return env; }
    sol::table& getClassTable() { return classTable; }
    sol::table& getInstanceTable() { return instanceTable; }
    [[nodiscard]] MSpatialEntity* getOwner() const { return owner; }

    // The context (and so the domain) that created this instance, or nullptr
    // once detached.
    [[nodiscard]] MLuaScriptContext* getContext() const { return context; }

    // Called by the owning context when it closes while this instance is
    // still alive: drops every Lua reference while the VM still exists and
    // marks the instance invalid, so later calls and release are safe no-ops.
    void detach();

private:
    sol::environment env;       // the class's environment (shared)
    sol::table classTable;      // the table returned by the script (shared)
    sol::table instanceTable;   // this entity's instance, passed as 'self'
    MSpatialEntity* owner;      // the owning entity
    MLuaScriptContext* context; // the context that created it - nullptr once detached
    bool valid = false;
};

#endif //LUA_SCRIPT_INSTANCE_H