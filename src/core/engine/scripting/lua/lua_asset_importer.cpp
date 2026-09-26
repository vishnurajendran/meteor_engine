//
// Created by ssj5v on 05-06-2026.
//

#include "lua_asset_importer.h"
#include "lua_script_asset.h"

bool MLuaScriptAssetImporter::registered = []()
{
    addImporter(new MLuaScriptAssetImporter());
    return true;
}();

MAsset* MLuaScriptAssetImporter::importAsset(SString path, const pugi::xml_document& metaData)
{
    auto* asset = new MLuaScriptAsset(path);
    return asset;
}
