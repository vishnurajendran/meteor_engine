//
// Created by ssj5v on 18-05-2026.
//

#include "audio_asset_importer.h"
#include "audioclip_asset.h"

bool MAudioAssetImporter::registeredAssetImporter = []()
{
    addImporter(new MAudioAssetImporter());
    return true;
}();

bool MAudioAssetImporter::canImport(SString fileExtension)
{
    return fileExtension == "wav" || fileExtension == "mp3" || fileExtension == "flac";
}

MAsset* MAudioAssetImporter::importAsset(SString path, const pugi::xml_document& metaData)
{
    auto* asset = new MAudioClipAsset(path);
    asset->loadSettings(metaData);
    return asset;
}