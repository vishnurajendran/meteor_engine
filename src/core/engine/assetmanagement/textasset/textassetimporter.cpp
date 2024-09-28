//
// Created by ssj5v on 27-09-2024.
//

#include "textassetimporter.h"

#include "textasset.h"

//registration pattern
bool MTextAssetImporter::registered = []() {
    addImporter(new MTextAssetImporter());
    return true;
}();

bool MTextAssetImporter::canImport(SString fileExtension) {
    return fileExtension == "txt";
}

MAsset * MTextAssetImporter::importAsset(SString path) {
    auto asset = new MTextAsset(path);
    if(!asset->loadFromPath(path)) {
        delete asset;
        return nullptr;
    }
    return asset;
}
