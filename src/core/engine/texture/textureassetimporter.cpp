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
    return fileExtension == "png" || fileExtension == "jpg";
}

MAsset * MTextureAssetImporter::importAsset(SString path) {
    MTextureAsset* asset = new MTextureAsset(path);
    if(!asset->isValid()) {
        delete asset;
        return nullptr;
    }
    return asset;
}
