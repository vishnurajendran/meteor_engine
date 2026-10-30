//
// Created by ssj5v on 18-05-2026.
//

#include "audio_asset_importer.h"
#include "audioclip_asset.h"
#include "core/memory/mnew.h"

bool MAudioAssetImporter::registeredAssetImporter = []()
{
    addImporter(mnew<MAudioAssetImporter>());
    return true;
}();

bool MAudioAssetImporter::canImport(SString fileExtension)
{
    return fileExtension == "wav" || fileExtension == "mp3" || fileExtension == "flac";
}

MAsset* MAudioAssetImporter::importAsset(SString path, const pugi::xml_document& metaData)
{
    auto* asset = mnew<MAudioClipAsset>(path);
    asset->loadSettings(metaData);
    return asset;
}