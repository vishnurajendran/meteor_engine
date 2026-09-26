#ifndef METEOR_LUA_BINDINGS_H
#define METEOR_LUA_BINDINGS_H

#include "core/utils/sstring_sol_traits.h"
#include "sol/sol.hpp"

class MLuaEngineBindings {
public:
    // Static method called by MLuaScriptingEngineSubsystem::init()
    static void BindEngineAPI(sol::state& vm);
};

#endif //METEOR_LUA_BINDINGS_H