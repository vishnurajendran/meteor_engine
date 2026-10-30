//
// Created by ssj5v on 06-10-2024.
//

#include "materialassetimporter.h"
#include "MMaterialAsset.h"
#include "core/memory/mnew.h"

const bool MMaterialAsserImporter::registered = []() {
    addImporter(mnew<MMaterialAsserImporter>());
    return true;
}();

bool MMaterialAsserImporter::canImport(SString fileExtension) {
    return fileExtension == "material";
}

MAsset * MMaterialAsserImporter::importAsset(SString path, const pugi::xml_document& metaData) {
    auto asset = mnew<MMaterialAsset>(path);
    if(!asset->isValid()) {
        delete asset;
        return nullptr;
    }
    return asset;
}
