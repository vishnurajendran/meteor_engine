//
// Created by ssj5v on 04-10-2024.
//

#ifndef SHADERASSETIMPORTER_H
#define SHADERASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"


class MShaderAssetImporter : public MAssetImporter {
private:
    static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return "meteor_assets/engine_assets/icons/file-shader.png";}
};



#endif //SHADERASSETIMPORTER_H
