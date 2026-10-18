//
// Created by ssj5v on 05-06-2026.
//

#include "lua_script_asset.h"

#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/scripting/lua/lua_script_registry.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"

MLuaScriptAsset::MLuaScriptAsset(const SString& path) : MAsset(path)
{
    loadSource();
    name = FileIO::getFileName(path);
}

MLuaScriptAsset::~MLuaScriptAsset()
{
    // A deleted script must stop resolving through import() — and its
    // compiled class must be dropped, which the registry change triggers.
    MLuaScriptRegistry::unregisterScript(this);
}

void MLuaScriptAsset::loadSource()
{
    valid = MAssetSources::getActive()->readText(path, scriptSrc);
    if (!valid)
    {
        MERROR("MLuaScriptAsset:: Failed to read script " + path);
        declaration = {};
        MLuaScriptRegistry::unregisterScript(this);
        return;
    }

    // Re-read on every load / hot reload, so renaming a class or namespace
    // takes effect on save.
    declaration = MLuaScriptRegistry::parseDeclaration(scriptSrc, path);
    MLuaScriptRegistry::registerScript(this);
}
