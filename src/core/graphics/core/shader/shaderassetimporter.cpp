//
// Created by ssj5v on 04-10-2024.
//

#include "shaderassetimporter.h"

#include "shaderasset.h"
#include "core/utils/logger.h"
#include "core/memory/mnew.h"

bool MShaderAssetImporter::registered = []() {
    addImporter(mnew<MShaderAssetImporter>());
    return true;
}();

bool MShaderAssetImporter::canImport(SString fileExtension) {
    return fileExtension == "mesl";
}

MAsset * MShaderAssetImporter::importAsset(SString path, const pugi::xml_document& metaData) {
    auto asset = mnew<MShaderAsset>(path);
    if(!asset->isValid()) {
        MERROR(STR("MShaderAssetImporter::importAsset(): invalid asset"));
        delete asset;
        return nullptr;
    }
    return asset;
}
