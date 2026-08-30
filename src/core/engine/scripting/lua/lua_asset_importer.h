//
// Created by ssj5v on 05-06-2026.
//

#ifndef LUA_ASSET_IMPORTER_H
#define LUA_ASSET_IMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"


class MLuaScriptAssetImporter : MAssetImporter {
    DEFINE_OBJECT_SUBCLASS(MLuaScriptAssetImporter)
public:
    bool canImport(SString fileExtension) override { return fileExtension == "lua"; }
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;

    SString getIconPath() const override { return SEngineAssetIconPaths::HIGHRES_TEX_ASSET_SCRIPT;}
private:
    static bool registered;
};



#endif //LUA_ASSET_IMPORTER_H
