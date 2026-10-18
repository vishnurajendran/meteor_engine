//
// script_access_bindings.h
//
// Adds getScript() to MSpatialEntity's Lua binding.
//
// Manual binding because getScript() returns a sol::environment
// which the annotation generator cannot handle.
// Must be registered AFTER the generated bindings.
//
// Returns the entity's script instance. Cast it to its class to use it —
// the cast returns nil if the script is something else:
//
//   local Enemy = import("game.enemies.Enemy")
//   local enemy = Enemy(other:getScript())
//   if enemy then enemy:takeDamage(10) end
//

#pragma once
#ifndef METEOR_SCRIPT_ACCESS_BINDINGS_H
#define METEOR_SCRIPT_ACCESS_BINDINGS_H

#include <sol/sol.hpp>
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scripting/lua/lua_script_instance.h"

namespace MeteorBindings {

    static void registerScriptAccessBindings(sol::state& lua) {
        lua["MSpatialEntity"]["getScript"] =
        [](MSpatialEntity* self, sol::this_state ts) -> sol::object
        {
            if (!self) return sol::lua_nil;

            auto* instance = self->getScriptInstance();
            if (!instance || !instance->isValid()) return sol::lua_nil;

            auto* luaInstance = dynamic_cast<MLuaScriptInstance*>(instance);
            if (!luaInstance) return sol::lua_nil;

            return luaInstance->getInstanceTable();
        };
    }

} // namespace MeteorBindings

#endif // METEOR_SCRIPT_ACCESS_BINDINGS_H