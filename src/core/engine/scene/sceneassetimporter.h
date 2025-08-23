//
// Created by ssj5v on 27-09-2024.
//

#ifndef SCENEASSETIMPORTER_H
#define SCENEASSETIMPORTER_H
#include "../assetmanagement/assetmanager/assetimporter.h"


class MSceneAssetImporter : MAssetImporter {
private:
    static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return "meteor_assets/engine_assets/icons/file-scene.png";}
};



#endif //SCENEASSETIMPORTER_H
