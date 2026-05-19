//
// Created by ssj5v on 04-10-2024.
//

#ifndef SHADERASSETIMPORTER_H
#define SHADERASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"

class MShaderAssetImporter : public MAssetImporter {
    DEFINE_OBJECT_SUBCLASS(MShaderAssetImporter)
private:
    static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return SEditorAssetPaths::HIGHRES_TEX_ASSET_SHADER; }
};



#endif //SHADERASSETIMPORTER_H
