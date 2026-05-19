//
// Created by ssj5v on 06-10-2024.
//

#ifndef MMATERIALASSETIMPORTER_H
#define MMATERIALASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"


class MMaterialAsserImporter : MAssetImporter {
    DEFINE_OBJECT_SUBCLASS(MMaterialAsserImporter)
    const static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return SEditorAssetPaths::HIGHRES_TEX_ASSET_MATERIAL; }
};

#endif //MMATERIALASSETIMPORTER_H
