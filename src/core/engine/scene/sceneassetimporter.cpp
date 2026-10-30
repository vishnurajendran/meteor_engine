//
// Created by ssj5v on 27-09-2024.
//

#include "sceneassetimporter.h"
#include "sceneasset.h"
#include "core/utils/logger.h"
#include "core/memory/mnew.h"

bool MSceneAssetImporter::registered = []() {
    addImporter(mnew<MSceneAssetImporter>());
    return true;
}();

bool MSceneAssetImporter::canImport(SString fileExtension) {
    return fileExtension == "scml";
}

MAsset * MSceneAssetImporter::importAsset(SString path, const pugi::xml_document& metaData) {
    auto asset = mnew<MSceneAsset>(path);
    if(!asset->isValid()) {
        MERROR(STR("MSceneAssetImporter::importAsset(): invalid asset"));
        delete asset;
        return nullptr;
    }
    return asset;
}
