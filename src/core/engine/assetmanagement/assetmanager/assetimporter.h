//
// Created by ssj5v on 27-09-2024.
//

#ifndef ASSETIMPORTER_H
#define ASSETIMPORTER_H
#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/object/object.h"
#include "pugixml.hpp"

/// Asset importer class. The Asset manager uses asset importers for
/// importing assets. each asset type requires its own individual asset importer.
class MAssetImporter : public MObject {
    DEFINE_OBJECT_SUBCLASS(MAssetImporter)
private:
    static std::vector<MAssetImporter*>* importers;

public:
    MAssetImporter() = default;
    ~MAssetImporter() override = default;

public:
    /// adds an importer to the global list
    static void addImporter(MAssetImporter* importer);
    /// get list of available importers
    static std::vector<MAssetImporter*>* getImporters();

    /// returns true if the importer can import the file with specified extension.
    virtual bool canImport(SString fileExtension) = 0;

    /// implemented by importers to handle asset import.\n
    /// parameter metaData is the xml tree for all settings required for import, and found in
    /// .meta file
    virtual MAsset* importAsset(SString path, const pugi::xml_document& metaData) = 0;

    /// returns the icon to represent this asset.\n
    /// used by **EDITOR** application
    virtual SString getIconPath() const;

    // special open requests
    /// attempts opening the asset\n
    /// used by **EDITOR** application
    virtual bool openAsset(MAsset* asset) { return false; }
};

#endif //ASSETIMPORTER_H
