//
// Created by ssj5v on 12-10-2024.
//

#include "textureassetimporter.h"
#include "textureasset.h"

const bool MTextureAssetImporter::registered = []() {
    addImporter(new MTextureAssetImporter());
    return true;
}();

bool MTextureAssetImporter::canImport(SString fileExtension) {
    return fileExtension == "png" || fileExtension == "jpg" || fileExtension == "jpeg";
}

MAsset* MTextureAssetImporter::importAsset(SString path, const pugi::xml_document& metaData)
{
    // Check for import settings in the meta file:
    //   <asset_id id="...">
    //       <textureImport>
    //           <filterMin>linear</filterMin>
    //           ...
    //       </textureImport>
    //   </asset_id>
    auto settingsNode = metaData.child("asset_id").child("textureImport");

    MTextureAsset* asset = settingsNode
        ? new MTextureAsset(path, settingsNode)
        : new MTextureAsset(path);

    if (!asset->isValid())
    {
        delete asset;
        return nullptr;
    }
    return asset;
}