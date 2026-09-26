//
// Created by ssj5v on 05-06-2026.
//

#ifndef LUA_SCRIPT_ASSET_H
#define LUA_SCRIPT_ASSET_H
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/scripting/interface/script_asset.h"
#include "core/utils/fileio.h"

SCRIPT_BIND_CLASS()
class MLuaScriptAsset : public MAsset, public IScriptAsset
{
    DEFINE_OBJECT_SUBCLASS(MLuaScriptAsset)
public:
    MLuaScriptAsset(const SString& path);
    ~MLuaScriptAsset() override = default;

    bool requestReload() override { loadSourceFromFile(); return valid; }
    SString getScriptSrc() const override { return scriptSrc; }

private:
    void loadSourceFromFile();
private:
    SString scriptSrc;
};

#endif //LUA_SCRIPT_ASSET_H
