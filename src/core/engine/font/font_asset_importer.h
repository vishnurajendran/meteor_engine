//
// font_asset_importer.h
//
// Imports .ttf and .otf files as MFontAsset.
//

#ifndef FONT_ASSET_IMPORTER_H
#define FONT_ASSET_IMPORTER_H

#include "core/engine/assetmanagement/assetmanager/assetimporter.h"

class MFontAssetImporter : public MAssetImporter
{
    DEFINE_OBJECT_SUBCLASS(MFontAssetImporter)
public:
    bool canImport(SString fileExtension) override;
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override;

private:
    static const bool registered;
};

#endif // FONT_ASSET_IMPORTER_H
