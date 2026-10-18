//
// Created by ssj5v on 05-06-2026.
//

#ifndef LUA_SCRIPT_ASSET_H
#define LUA_SCRIPT_ASSET_H
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/scripting/interface/script_asset.h"
#include "core/engine/scripting/lua/lua_script_declaration.h"
#include "core/utils/fileio.h"

SCRIPT_BIND_CLASS()
class MLuaScriptAsset : public MAsset, public IScriptAsset
{
    DEFINE_OBJECT_SUBCLASS(MLuaScriptAsset)
public:
    MLuaScriptAsset(const SString& path);
    ~MLuaScriptAsset() override;

    bool requestReload() override { loadSource(); return valid; }
    SString getScriptSrc() const override { return scriptSrc; }

    // The namespace / class name read from the script header — what
    // import() finds it by. See MLuaScriptRegistry.
    [[nodiscard]] const SLuaScriptDeclaration& getDeclaration() const { return declaration; }

private:
    void loadSource();
private:
    SString scriptSrc;
    SLuaScriptDeclaration declaration;
};

#endif //LUA_SCRIPT_ASSET_H
