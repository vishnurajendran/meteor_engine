//
// Created by ssj5v on 06-10-2024.
//

#ifndef MMATERIALASSETIMPORTER_H
#define MMATERIALASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"


class MMaterialAsserImporter : MAssetImporter {
    const static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path) override;
};

#endif //MMATERIALASSETIMPORTER_H
