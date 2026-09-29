//
// Created by ssj5v on 05-06-2026.
//

#include "lua_script_asset.h"

#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"

MLuaScriptAsset::MLuaScriptAsset(const SString& path) : MAsset(path)
{
    loadSource();
    name = FileIO::getFileName(path);
}


void MLuaScriptAsset::loadSource()
{
    valid = MAssetSources::getActive()->readText(path, scriptSrc);
    if (!valid)
    {
        MERROR("MLuaScriptAsset:: Failed to read script " + path);
    }
}
