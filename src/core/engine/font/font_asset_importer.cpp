//
// font_asset_importer.cpp
//

#include "font_asset_importer.h"

#include "core/memory/mnew.h"
#include "default_engine_icon_paths.h"
#include "font_asset.h"
#include "pugixml.hpp"

const bool MFontAssetImporter::registered = []() {
    addImporter(mnew<MFontAssetImporter>());
    return true;
}();

bool MFontAssetImporter::canImport(SString fileExtension)
{
    return fileExtension == "ttf" || fileExtension == "otf";
}

MAsset* MFontAssetImporter::importAsset(SString path, const pugi::xml_document& metaData)
{
    const auto settings = metaData.child("asset_id").child("fontImport");
    auto* asset = settings ? mnew<MFontAsset>(path, settings) : mnew<MFontAsset>(path);
    if (!asset->isValid())
    {
        delete asset;
        return nullptr;
    }
    return asset;
}

SString MFontAssetImporter::getIconPath() const
{
    return SEngineAssetIconPaths::HIGHRES_TEX_ASSET_FONT;
}
