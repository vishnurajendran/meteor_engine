//
// Created by ssj5v on 23-08-2026.
//

#ifndef SCRIPTING_CALL_SYMBOLS_H
#define SCRIPTING_CALL_SYMBOLS_H
#include "core/utils/sstring.h"

namespace ScriptinCallSymbols
{
    const SString SCRIPT_FUNC_START = "onStart";
    const SString SCRIPT_FUNC_STOP = "onStop";
    const SString SCRIPT_FUNC_TICK = "onTick";
    const SString SCRIPT_FUNC_FIXED_TICK = "onFixedTick";
    const SString SCRIPT_FUNC_LATE_TICK = "onLateTick";

    // Physics messages - called on the collision body's own script, only if
    // the script defines them (Unity-style). Engine events Start/End map to
    // Enter/Exit here.
    const SString SCRIPT_FUNC_COLLISION_ENTER = "onCollisionEnter";
    const SString SCRIPT_FUNC_COLLISION_STAY  = "onCollisionStay";
    const SString SCRIPT_FUNC_COLLISION_EXIT  = "onCollisionExit";
    const SString SCRIPT_FUNC_TRIGGER_ENTER   = "onTriggerEnter";
    const SString SCRIPT_FUNC_TRIGGER_STAY    = "onTriggerStay";
    const SString SCRIPT_FUNC_TRIGGER_EXIT    = "onTriggerExit";

}

#endif //SCRIPTING_CALL_SYMBOLS_H
