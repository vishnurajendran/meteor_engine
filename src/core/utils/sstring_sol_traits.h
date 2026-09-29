//
// sstring_sol_traits.h
//
// Teaches Sol2 how to push/pull SString to/from the Lua stack.
// With this, any SCRIPT_BIND_FUNC that takes or returns SString
// works transparently with Lua strings.
//
// Must be included AFTER sol/sol.hpp and BEFORE any binding code
// that uses SString with Sol2.
//

#pragma once
#ifndef SSTRING_SOL_TRAITS_H
#define SSTRING_SOL_TRAITS_H

#include "sol/sol.hpp"
#include "core/utils/sstring.h"

namespace sol {

// Tell Sol2 that SString maps to the Lua string type, not userdata
template <>
struct lua_type_of<SString> : std::integral_constant<sol::type, sol::type::string> {};

// Prevent Sol2 from auto-generating a usertype for SString
template <>
struct is_automagical<SString> : std::false_type {};

namespace stack {

    // Push SString onto the Lua stack as a Lua string
    template <>
    struct unqualified_pusher<SString> {
        static int push(lua_State* L, const SString& str) {
            const auto& s = str.str();
            lua_pushlstring(L, s.c_str(), s.size());
            return 1;
        }
    };

    // Read SString from the Lua stack
    template <>
    struct unqualified_getter<SString> {
        static SString get(lua_State* L, int index, record& tracking) {
            tracking.use(1);
            size_t len = 0;
            const char* str = lua_tolstring(L, index, &len);
            return SString(str ? std::string(str, len) : std::string());
        }
    };

    // Check if the value at the stack index can be a SString
    template <>
    struct unqualified_checker<SString, type::string> {
        template <typename Handler>
        static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
            tracking.use(1);
            bool success = lua_isstring(L, index) != 0;
            if (!success) {
                handler(L, index, type::string, type_of(L, index), "expected a string");
            }
            return success;
        }
    };

} // namespace stack
} // namespace sol

#endif // SSTRING_SOL_TRAITS_H