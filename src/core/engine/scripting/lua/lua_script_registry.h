//
// lua_script_registry.h
//
// Index of every loaded Lua script asset by its declared full name - what
// import() resolves against.
//
// The full name is read from the script's header as text (see
// lua_script_declaration.h); the script is never executed to find it. That
// keeps the index complete before any script runs, so import() works no
// matter what order scripts load in, and a bad declaration is reported as
// soon as the file is saved - not when play mode reaches it.
//
// Every register / unregister bumps a generation counter:
//   - the Lua engine compares it to its own and drops its compiled class
//     cache when it moved (any script change flushes every class)
//   - the editor compares it to rewrite the VS Code import index
//

#ifndef LUA_SCRIPT_REGISTRY_H
#define LUA_SCRIPT_REGISTRY_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/engine/scripting/bridge/dyn_value.h"
#include "lua_script_declaration.h"

class MLuaScriptAsset;

// A public field of a script class, as the inspector shows it: names not
// starting with '_' whose value is a boolean, number or string. The type is
// the default's type (Integer for 5, Float for 5.0).
struct SLuaScriptField
{
    SString   name;
    SDynValue defaultValue;
};

// What the editor's script refresh found for one script.
struct SLuaScriptFieldSet
{
    bool compiled = false;               // false: the script failed to compile
    std::vector<SLuaScriptField> fields; // inherited fields first, then the class's own, in source order
};

class MLuaScriptRegistry
{
public:
    static constexpr const char* DEFAULT_NAMESPACE = "meteor.game";

    // Reads MODULE / CLASS and the class's member table from a
    // script's source, without running it. Problems (a non-literal name, a
    // MODULE after the class, two classes) are logged against scriptPath, and
    // the script is reported as not importable rather than guessed at.
    // The engine checks the same names again when the script actually runs.
    static SLuaScriptDeclaration parseDeclaration(const SString& source, const SString& scriptPath);

    // Indexes the asset under its current declaration, replacing any earlier
    // entry for it. Call after every (re)load of its source.
    static void registerScript(MLuaScriptAsset* asset);
    static void unregisterScript(MLuaScriptAsset* asset);

    // Every script declaring fullName - empty if unknown, more than one if
    // the name clashes. Clashes are never resolved silently.
    static std::vector<MLuaScriptAsset*> find(const SString& fullName);

    // Every loaded script, importable or not, sorted by path - what the
    // script refresh compiles.
    static std::vector<MLuaScriptAsset*> getAllScripts();
    static MLuaScriptAsset* findByPath(const SString& path);

    // Public fields, published by the editor's script refresh from the
    // compiled class. nullptr until the script has been refreshed once.
    static void setPublicFields(MLuaScriptAsset* asset, SLuaScriptFieldSet fields);
    [[nodiscard]] static const SLuaScriptFieldSet* getPublicFields(const MLuaScriptAsset* asset);

    // The text of VS Code's script_index.lua: one ---@class block per
    // importable script (base, cast overload, members) and a typed import()
    // overload per full name. Sorted by full name; a clashing name is listed once.
    static std::string buildTypeIndex();

    static uint64_t getGeneration() { return generation; }

private:
    static void removeEntry(MLuaScriptAsset* asset);

    static std::unordered_map<SString, std::vector<MLuaScriptAsset*>> scriptsByName;
    static std::unordered_map<MLuaScriptAsset*, SString> nameByScript;
    static std::unordered_set<MLuaScriptAsset*> allScripts;
    static std::unordered_map<const MLuaScriptAsset*, SLuaScriptFieldSet> publicFields;
    static uint64_t generation;
};

#endif // LUA_SCRIPT_REGISTRY_H
