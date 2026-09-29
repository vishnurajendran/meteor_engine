// LuaTypeMap.cs
// Maps C++ types to their Lua C API push/check function names.
//
// When the code emitter generates a getter, it needs to know:
//   "int" -> lua_pushinteger / luaL_checkinteger
//   "float" -> lua_pushnumber / (float)luaL_checknumber
//   "bool" -> lua_pushboolean / lua_toboolean
//
// This file centralizes that mapping so the emitter stays clean.

using System.Collections.Generic;

namespace MeteorBindingGenerator;

/// <summary>
/// Describes how to push a C++ value onto the Lua stack
/// and how to read it back from the Lua stack.
/// </summary>
public record LuaTypeMapping(
    /// <summary>Lua C API call to push the value, e.g. "lua_pushinteger"</summary>
    string PushCall,
    /// <summary>Lua C API call to read the value, e.g. "luaL_checkinteger"</summary>
    string CheckCall,
    /// <summary>Optional cast to apply when reading, e.g. "(float)" for float from luaL_checknumber</summary>
    string ReadCast,
    /// <summary>Whether this is a primitive type with direct push/check support</summary>
    bool IsPrimitive
);

public static class LuaTypeMap
{
    private static readonly Dictionary<string, LuaTypeMapping> Mappings = new()
    {
        ["int"]            = new("lua_pushinteger",  "luaL_checkinteger",  "(int)",    true),
        ["short"]          = new("lua_pushinteger",  "luaL_checkinteger",  "(short)",  true),
        ["long"]           = new("lua_pushinteger",  "luaL_checkinteger",  "(long)",   true),
        ["long long"]      = new("lua_pushinteger",  "luaL_checkinteger",  "",         true),
        ["unsigned int"]   = new("lua_pushinteger",  "luaL_checkinteger",  "(unsigned int)", true),
        ["float"]          = new("lua_pushnumber",   "luaL_checknumber",   "(float)",  true),
        ["double"]         = new("lua_pushnumber",   "luaL_checknumber",   "",         true),
        ["bool"]           = new("lua_pushboolean",  "lua_toboolean",      "",         true),
        ["std::string"]    = new("lua_pushstring",   "luaL_checkstring",   "",         true),
        ["SString"]        = new("lua_pushstring",   "luaL_checkstring",   "",         true),
        ["char"]           = new("lua_pushinteger",  "luaL_checkinteger",  "(char)",   true),
    };

    /// <summary>
    /// Look up how to push/check a C++ type to/from the Lua stack.
    /// Returns null if the type is not a known primitive (meaning it
    /// needs userdata handling or a custom conversion).
    /// </summary>
    public static LuaTypeMapping? GetMapping(string cppType)
    {
        return Mappings.GetValueOrDefault(cppType);
    }

    /// <summary>
    /// Check whether a type is a known primitive that maps directly
    /// to a Lua push/check call.
    /// </summary>
    public static bool IsPrimitive(string cppType)
    {
        return Mappings.ContainsKey(cppType);
    }

    /// <summary>
    /// Generate the C++ expression to push a value onto the Lua stack.
    /// For std::string/SString, appends .c_str().
    /// </summary>
    public static string GeneratePush(string cppType, string valueExpr)
    {
        var mapping = GetMapping(cppType);
        if (mapping == null)
            return $"// TODO: push for type '{cppType}' not implemented";

        var arg = valueExpr;
        if (cppType == "std::string" || cppType == "SString")
            arg += ".c_str()";

        return $"{mapping.PushCall}(L, {arg})";
    }

    /// <summary>
    /// Generate the C++ expression to read a value from the Lua stack.
    /// Includes any necessary casts.
    /// </summary>
    public static string GenerateCheck(string cppType, int stackIndex)
    {
        var mapping = GetMapping(cppType);
        if (mapping == null)
            return $"/* TODO: check for type '{cppType}' not implemented */";

        var checkExpr = $"{mapping.CheckCall}(L, {stackIndex})";

        if (!string.IsNullOrEmpty(mapping.ReadCast))
            return $"{mapping.ReadCast}{checkExpr}";

        return checkExpr;
    }
}
