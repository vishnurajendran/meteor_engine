//
// Created by ssj5v on 05-06-2026.
//

#include "lua_asset_importer.h"
#include "lua_script_asset.h"
#include "core/memory/mnew.h"

bool MLuaScriptAssetImporter::registered = []()
{
    addImporter(mnew<MLuaScriptAssetImporter>());
    return true;
}();

MAsset* MLuaScriptAssetImporter::importAsset(SString path, const pugi::xml_document& metaData)
{
    auto* asset = mnew<MLuaScriptAsset>(path);
    return asset;
}
