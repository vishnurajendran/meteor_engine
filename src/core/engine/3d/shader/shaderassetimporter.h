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
    MAsset * importAsset(SString path) override;
};



#endif //SHADERASSETIMPORTER_H
