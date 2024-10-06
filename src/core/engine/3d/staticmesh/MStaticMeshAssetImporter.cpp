//
// Created by ssj5v on 29-09-2024.
//

#include "MStaticMeshAssetImporter.h"

#include "core/utils/logger.h"

bool MStaticMeshAssetImporter::registered = []() {
    addImporter(new MStaticMeshAssetImporter());
    return true;
}();

bool MStaticMeshAssetImporter::canImport(SString fileExtension) {
    std::vector<SString> supportedFileExtensions = { "gltf", "glb", "obj", "fbx" };
    return std::find(supportedFileExtensions.begin(), supportedFileExtensions.end(), fileExtension)
                                                                            != supportedFileExtensions.end();
}

MAsset * MStaticMeshAssetImporter::importAsset(SString path) {
    auto asset = new MStaticMeshAsset(path);
    if(!asset->isValid()) {
        MERROR(STR("MStaticMeshAssetImporter::importAsset(): invalid asset"));
        delete asset;
        return nullptr;
    }
    return asset;
}

