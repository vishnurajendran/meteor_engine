//
// Created by ssj5v on 27-09-2024.
//

#include "sceneassetimporter.h"
#include "sceneasset.h"

bool MSceneAssetImporter::registered = []() {
    addImporter(new MSceneAssetImporter());
    return true;
}();

bool MSceneAssetImporter::canImport(SString fileExtension) {
    return fileExtension == "scml";
}

MAsset * MSceneAssetImporter::importAsset(SString path) {
    auto asset = new MSceneAsset(path);
    if(!asset->isValid()) {
        delete asset;
        return nullptr;
    }
    return asset;
}
