//
// Created by ssj5v on 27-09-2024.
//

#ifndef ASSETIMPORTER_H
#define ASSETIMPORTER_H
#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/object/object.h"
#include "pugixml.hpp"


class MAssetImporter : MObject {
private:
    static std::vector<MAssetImporter*>* importers;
public:
    static void addImporter(MAssetImporter* importer);
    static std::vector<MAssetImporter*>* getImporters();
    virtual bool canImport(SString fileExtension) = 0;

    virtual MAsset* importAsset(SString path, const pugi::xml_document& metaData) = 0;
    virtual SString getIconPath() const;

    // special open requests
    virtual bool openAsset(MAsset* asset) { return false; }
};

#endif //ASSETIMPORTER_H
