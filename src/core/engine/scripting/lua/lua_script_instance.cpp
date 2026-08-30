// lua_script_instance.cpp
#include "lua_script_instance.h"
#include "core/utils/logger.h"
#include <string>

// --- Helper Functions for SDynValue <-> sol::object ---

static sol::object DynValueToSolObject(const SDynValue& val, sol::state_view vm) {
    switch(val.type) {
        case EDynValueType::Integer: return sol::make_object(vm, std::get<int>(val.value));
        case EDynValueType::Float:   return sol::make_object(vm, std::get<float>(val.value));
        case EDynValueType::Bool:    return sol::make_object(vm, std::get<bool>(val.value));
        case EDynValueType::String:  return sol::make_object(vm, std::get<std::string>(val.value));
        default:      return sol::lua_nil;
    }
}

static SDynValue SolObjectToDynValue(const sol::object& obj) {
    SDynValue result;
    result.type = EDynValueType::NoValue;

    if (obj.is<int>())               { result.setInt(obj.as<int>()); }
    else if (obj.is<float>())        { result.setFloat(obj.as<float>()); }
    else if (obj.is<bool>())         { result.setBool(obj.as<bool>()); }
    else if (obj.is<std::string>())  { result.setString(obj.as<std::string>()); }

    return result;
}

static bool handleFuncResult(const sol::protected_function_result& funcResult, SDynValue& result)
{
    if (!funcResult.valid())
    {
        sol::error err = funcResult;
        MERROR(err.what());
        return false;
    }

    if (funcResult.get_type() != sol::type::none && funcResult.get_type() != sol::type::lua_nil)
        result = SolObjectToDynValue(funcResult.get<sol::object>());
    else
        result.type = EDynValueType::NoValue;

    return true;
}

// --- Instance Implementation ---

MLuaScriptInstance::MLuaScriptInstance(sol::state& vm, const SString& scriptSrc, MSpatialEntity* owner)
    : owner(owner)
{
    env = sol::environment(vm, sol::create, vm.globals());

    auto load_result = vm.script(scriptSrc.c_str(), env, sol::script_pass_on_error);

    if (!load_result.valid())
    {
        sol::error err = load_result;
        MERROR(err.what());
        valid = false;
        return;
    }

    if (load_result.get_type() != sol::type::table)
    {
        MERROR("Script must return a table. Ensure your script ends with 'return ClassName'");
        valid = false;
        return;
    }

    classTable = load_result.get<sol::table>();
    classTable["__entity"] = owner;
    valid = true;
}

// Named-parameter form
bool MLuaScriptInstance::callFunc(const SString& funcName, const std::unordered_map<SString, SDynValue>& params, SDynValue& result)
{
    if (!valid || !classTable.valid()) {
        return false;
    }

    sol::protected_function func = classTable[funcName.c_str()];
    sol::state_view vview = env.lua_state();

    sol::table luaParams = vview.create_table();
    for (const auto& kv : params)
        luaParams[kv.first.c_str()] = DynValueToSolObject(kv.second, vview);

    // Pass the class table as first arg (becomes 'self' in Lua)
    return handleFuncResult(func(classTable, luaParams), result);
}

// Positional-argument form
bool MLuaScriptInstance::callFunc(const SString& funcName, const std::vector<SDynValue>& args, SDynValue& result)
{
    if (!valid || !classTable.valid()) return false;
    sol::protected_function func = classTable[funcName.c_str()];
    if (!func.valid()) return false;

    sol::state_view view = env.lua_state();

    // Convert SDynValue args to sol::objects
    std::vector<sol::object> solArgs;
    solArgs.reserve(args.size());
    for (const auto& arg : args)
        solArgs.push_back(DynValueToSolObject(arg, view));

    // Call with class table as self, then remaining args
    sol::protected_function_result funcResult;
    if (solArgs.empty())
    {
        funcResult = func(classTable);
    }
    else if (solArgs.size() == 1)
    {
        funcResult = func(classTable, solArgs[0]);
    }
    else if (solArgs.size() == 2)
    {
        funcResult = func(classTable, solArgs[0], solArgs[1]);
    }
    else if (solArgs.size() == 3)
    {
        funcResult = func(classTable, solArgs[0], solArgs[1], solArgs[2]);
    }
    else
    {
        funcResult = func(classTable, sol::as_args(solArgs));
    }

    return handleFuncResult(funcResult, result);
}

void MLuaScriptInstance::setEnvVar(const SString& name, const SDynValue& value)
{
    sol::state_view vview = env.lua_state();
    env[name.c_str()] = DynValueToSolObject(value, vview);
}