//
// lua_script_declaration.h
//
// What a Lua script declares about itself, read from its source text:
//
//   MODULE "game.camera"                                   (optional — default "meteor.game")
//   local CameraController = CLASS("CameraController", Behaviour) {
//       speed = 5,
//       move = function(self, x, y) ... end,
//   }
//
//   → fullName "game.camera.CameraController", what import() resolves,
//     plus the base class and members the editor writes into VS Code's
//     type index.
//
// Kept in its own header — with only <vector> and sstring.h — because
// lua_script_asset.h is parsed by the binding generator against stub
// headers, and anything it includes must resolve there.
//

#ifndef LUA_SCRIPT_DECLARATION_H
#define LUA_SCRIPT_DECLARATION_H

#include <vector>

#include "core/utils/sstring.h"

// A top-level member of the class's member table, as VS Code's index declares it.
struct SLuaScriptMember
{
    SString name;
    SString luaType;   // a LuaLS type, e.g. "number" or "fun(self: CameraController, x: number)"
};

struct SLuaScriptDeclaration
{
    SString nameSpace;      // "meteor.game" when the script has no MODULE
    SString className;      // from CLASS — empty if there is none
    SString fullName;       // nameSpace + "." + className — empty if not importable
    SString baseClassName;  // LuaLS type of the base: "Behaviour" or an imported class's name

    std::vector<SLuaScriptMember> members;

    [[nodiscard]] bool isImportable() const { return !fullName.empty(); }
};

#endif // LUA_SCRIPT_DECLARATION_H
