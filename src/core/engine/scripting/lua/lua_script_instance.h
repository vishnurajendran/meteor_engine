// lua_script_instance.h
#ifndef LUA_SCRIPT_INSTANCE_H
#define LUA_SCRIPT_INSTANCE_H

#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scripting/interface/script_instance.h"
#include "core/object/object.h"
#include "core/object/object_class_macros.h"
#include "sol/sol.hpp"

class MLuaScriptInstance : public MObject, public IScriptInstance
{
    DEFINE_OBJECT_SUBCLASS(MLuaScriptInstance)
public:
    MLuaScriptInstance(sol::state& vm, const SString& scriptSrc, MSpatialEntity* owner);
    ~MLuaScriptInstance() override = default;

    bool isValid() const override { return valid; }

    bool callFunc(const SString& funcName, const std::unordered_map<SString, SDynValue>& params, SDynValue& result) override;
    bool callFunc(const SString& funcName, const std::vector<SDynValue>& args, SDynValue& result) override;

    void setEnvVar(const SString& name, const SDynValue& value) override;

    template<typename T>
    void setEnvVarTyped(const SString& name, T value)
    {
        env[name.c_str()] = value;
    }

    sol::environment& getEnvironment() { return env; }
    sol::table& getClassTable() { return classTable; }
    [[nodiscard]] MSpatialEntity* getOwner() const { return owner; }

private:
    sol::environment env;
    sol::table classTable;      // the table returned by the script
    MSpatialEntity* owner;      // the owning entity, passed as 'self'
    bool valid = false;
};

#endif //LUA_SCRIPT_INSTANCE_H