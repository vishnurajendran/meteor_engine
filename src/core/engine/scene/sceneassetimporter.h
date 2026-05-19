//
// Created by ssj5v on 27-09-2024.
//

#ifndef SCENEASSETIMPORTER_H
#define SCENEASSETIMPORTER_H
#include "../assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"


class MSceneAssetImporter : public MAssetImporter {
    DEFINE_OBJECT_SUBCLASS(MSceneAssetImporter)
private:
    static bool registered;
public:
    MSceneAssetImporter() = default;
    ~MSceneAssetImporter() override = default;

    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return SEditorAssetPaths::HIGHRES_TEX_ASSET_SCENE; }
};



#endif //SCENEASSETIMPORTER_H
