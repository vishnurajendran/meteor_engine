//
// Created by ssj5v on 04-06-2026.
//

#ifndef SCRIPT_H
#define SCRIPT_H
#include "core/engine/scripting/bridge/dyn_value.h"
#include "core/utils/sstring.h"

class IScriptInstance
{
public:
    IScriptInstance() = default;
    virtual ~IScriptInstance() = default;
    virtual bool isValid() const = 0;

    // Named-parameter form: values are packed into a single Lua table argument.
    virtual bool callFunc(const SString& funcName, const std::unordered_map<SString, SDynValue>& params, SDynValue& result) = 0;

    // Positional-argument form: each SDynValue is passed as a separate Lua argument.
    virtual bool callFunc(const SString& funcName, const std::vector<SDynValue>& args, SDynValue& result) = 0;

    // Set a variable in the script's environment using a dynamic value.
    // Covers int, float, bool, string via SDynValue.
    virtual void setEnvVar(const SString& name, const SDynValue& value) = 0;

    // Behaviour tuning: sets a value on this instance only, shadowing the
    // script's default for this entity. Fails (returns false) when the script
    // has no public field of that name or the value's type does not match it.
    virtual bool setField(const SString& name, const SDynValue& value) = 0;

    // Removes this instance's own value, so the script's default shows through.
    virtual void clearField(const SString& name) = 0;
};

#endif //SCRIPT_H