//
// Created by ssj5v on 29-09-2024.
//

#ifndef MMESHASSETIMPORTER_H
#define MMESHASSETIMPORTER_H
#include "staticmeshasset.h"
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"

class MStaticMeshAssetImporter : public MAssetImporter {
private:
    static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path) override;
};

#endif //MMESHASSETIMPORTER_H
