//
// Created by ssj5v on 29-09-2024.
//

#include "MStaticMeshAssetImporter.h"

#include "core/utils/logger.h"
#include "core/memory/mnew.h"

bool MStaticMeshAssetImporter::registered = []() {
    addImporter(mnew<MStaticMeshAssetImporter>());
    return true;
}();

bool MStaticMeshAssetImporter::canImport(SString fileExtension) {
    std::vector<SString> supportedFileExtensions = { "gltf", "glb", "obj", "fbx" };
    return std::find(supportedFileExtensions.begin(), supportedFileExtensions.end(), fileExtension)
                                                                            != supportedFileExtensions.end();
}

MAsset * MStaticMeshAssetImporter::importAsset(SString path, const pugi::xml_document& metaData) {
    // Import settings live in the meta file:
    //   <asset_id id="..."><meshImport><scaleFactor>0.01</scaleFactor></meshImport></asset_id>
    auto settingsNode = metaData.child("asset_id").child("meshImport");
    auto asset = settingsNode
        ? mnew<MStaticMeshAsset>(path, settingsNode)
        : mnew<MStaticMeshAsset>(path);
    if(!asset->isValid()) {
        MERROR(STR("MStaticMeshAssetImporter::importAsset(): invalid asset"));
        delete asset;
        return nullptr;
    }
    return asset;
}

