//
// Created by ssj5v on 12-10-2024.
//

#ifndef TEXTUREASSETIMPORTER_H
#define TEXTUREASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"


class MTextureAsset;

class MTextureAssetImporter : public MAssetImporter {
private:
    static const bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path) override;
};



#endif //TEXTUREASSETIMPORTER_H
