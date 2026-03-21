//
// Created by ssj5v on 30-03-2025.
//

#ifndef CUBEMAPASSETIMPORTER_H
#define CUBEMAPASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"


class MCubemapAssetImporter : public MAssetImporter {
public:
    bool canImport(SString fileExtension) override;
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return "meteor_assets/engine_assets/icons/file-skybox.png";}
private:
    static const bool registered;
};



#endif //CUBEMAPASSETIMPORTER_H
