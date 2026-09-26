#include <lua.h>
#include <sol/sol.hpp>
#include "core/utils/logger.h"

namespace MeteorBindings
{
    /**
     * Helper function to extract runtime debug information from the Lua stack.
     * Uses level 1 to see exactly who called the C++ logger bridge function.
     */
    static SLogLocation getLuaSourceLocation(lua_State* L)
    {
        lua_Debug ar;
        // "S" gets the source/file name, "l" gets the current line, "n" gets the function name
        if (lua_getstack(L, 1, &ar) && lua_getinfo(L, "Sln", &ar))
        {
            const char* file = "Lua Script";
            int line = ar.currentline > 0 ? ar.currentline : 0;
            const char* func = ar.namewhat ? ar.namewhat : "??";

            return SLogLocation{file, line, func};
        }

        // Fallback if the Lua stack state cannot be inspected
        return SLogLocation{"Lua", 0, "???"};
    }

    /**
     * Registers the engine's MLogger methods into the Lua global state.
     */
    static void registerLoggerModule(sol::state& lua)
    {
        // Create a dedicated namespace table
        sol::table logger = lua.create_table();

        // Bind: logger.info("message")
        logger["info"] = [](sol::this_state ts, const std::string& msg)
        {
            SLogLocation loc = getLuaSourceLocation(ts);
            MLogger::log(SString(msg.c_str()), loc);
        };

        // Bind: logger.verbose("message")
        logger["verbose"] = [](sol::this_state ts, const std::string& msg)
        {
            SLogLocation loc = getLuaSourceLocation(ts);
            MLogger::verbose(SString(msg.c_str()), loc);
        };

        // Bind: logger.warn("message")
        logger["warn"] = [](sol::this_state ts, const std::string& msg)
        {
            SLogLocation loc = getLuaSourceLocation(ts);
            MLogger::warn(SString(msg.c_str()), loc);
        };

        // Bind: logger.error("message")
        logger["error"] = [](sol::this_state ts, const std::string& msg)
        {
            SLogLocation loc = getLuaSourceLocation(ts);
            MLogger::error(SString(msg.c_str()), loc);
        };

        // Expose the namespace table globally to the Lua Environment
        lua["MLogger"] = logger;
    }

} // namespace MeteorEngine
