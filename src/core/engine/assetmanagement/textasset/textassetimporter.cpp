//
// Created by ssj5v on 27-09-2024.
//

#include "textassetimporter.h"

#include "textasset.h"
#include "core/utils/logger.h"
#include "core/memory/mnew.h"

//registration pattern
bool MTextAssetImporter::registered = []() {
    addImporter(mnew<MTextAssetImporter>());
    return true;
}();

bool MTextAssetImporter::canImport(SString fileExtension) {
    return fileExtension == "txt";
}

MAsset * MTextAssetImporter::importAsset(SString path, const pugi::xml_document& metaData) {
    auto asset = mnew<MTextAsset>(path);
    if(!asset->isValid()) {
        MERROR(STR("MStaticMeshAssetImporter::importAsset(): invalid asset"));
        delete asset;
        return nullptr;
    }
    return asset;
}
