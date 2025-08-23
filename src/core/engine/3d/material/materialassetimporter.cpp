//
// Created by ssj5v on 06-10-2024.
//

#include "materialassetimporter.h"
#include "MMaterialAsset.h"

const bool MMaterialAsserImporter::registered = []() {
    addImporter(new MMaterialAsserImporter());
    return true;
}();

bool MMaterialAsserImporter::canImport(SString fileExtension) {
    return fileExtension == "material";
}

MAsset * MMaterialAsserImporter::importAsset(SString path, const pugi::xml_document& metaData) {
    auto asset = new MMaterialAsset(path);
    if(!asset->isValid()) {
        delete asset;
        return nullptr;
    }
    return asset;
}
