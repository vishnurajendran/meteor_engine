//
// Created by ssj5v on 27-09-2024.
//

#ifndef TEXTASSETIMPORTER_H
#define TEXTASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"


class MTextAssetImporter : MAssetImporter {
private:
    static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path) override;
};



#endif //TEXTASSETIMPORTER_H
