//
// Created by ssj5v on 29-09-2024.
//

#ifndef MMESHASSETIMPORTER_H
#define MMESHASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"
#include "staticmeshasset.h"

class MStaticMeshAssetImporter : public MAssetImporter {
    DEFINE_OBJECT_SUBCLASS(MStaticMeshAssetImporter)
private:
    static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return SEditorAssetPaths::HIGHRES_TEX_ASSET_STATICMESH; }
};

#endif //MMESHASSETIMPORTER_H
