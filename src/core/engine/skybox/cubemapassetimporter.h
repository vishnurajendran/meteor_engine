//
// Created by ssj5v on 30-03-2025.
//

#ifndef CUBEMAPASSETIMPORTER_H
#define CUBEMAPASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"


class MCubemapAssetImporter : public MAssetImporter {
public:
    bool canImport(SString fileExtension) override;
    MAsset* importAsset(SString path) override;
private:
    static const bool registered;
};



#endif //CUBEMAPASSETIMPORTER_H
