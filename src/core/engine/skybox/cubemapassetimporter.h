//
// Created by ssj5v on 30-03-2025.
//

#ifndef CUBEMAPASSETIMPORTER_H
#define CUBEMAPASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"


class MCubemapAssetImporter : public MAssetImporter {
    DEFINE_OBJECT_SUBCLASS(MCubemapAssetImporter)
public:
    bool canImport(SString fileExtension) override;
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return SEditorAssetPaths::HIGHRES_TEX_ASSET_SKYBOX; }
private:
    static const bool registered;
};



#endif //CUBEMAPASSETIMPORTER_H
