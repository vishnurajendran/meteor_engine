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

}

#endif //SCRIPTING_CALL_SYMBOLS_H
