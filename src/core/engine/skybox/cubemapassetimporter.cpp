//
// Created by ssj5v on 30-03-2025.
//

#include "cubemapassetimporter.h"
#include "core/utils/logger.h"
#include "cubemapasset.h"

const bool MCubemapAssetImporter::registered = []() {
    addImporter(new MCubemapAssetImporter());
    return true;
}();

bool MCubemapAssetImporter::canImport(SString fileExtension) {
    return fileExtension == "skybox";
}

MAsset * MCubemapAssetImporter::importAsset(SString path) {
    MCubemapAsset* asset = new MCubemapAsset(path);
    if(!asset->isValid()) {
        delete asset;
        return nullptr;
    }
    return asset;
}