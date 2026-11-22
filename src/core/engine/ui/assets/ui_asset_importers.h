//
// ui_asset_importers.h
//
// Importers for the UI asset types:
//   .rml   MUIStructureAsset
//   .rcss  MUIStyleAsset
//   .muia  MUIWidgetAsset
//

#ifndef UI_ASSET_IMPORTERS_H
#define UI_ASSET_IMPORTERS_H

#include "core/engine/assetmanagement/assetmanager/assetimporter.h"

class MUIStructureAssetImporter : public MAssetImporter
{
    DEFINE_OBJECT_SUBCLASS(MUIStructureAssetImporter)
public:
    bool canImport(SString fileExtension) override { return fileExtension == "rml"; }
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override;
};

class MUIStyleAssetImporter : public MAssetImporter
{
    DEFINE_OBJECT_SUBCLASS(MUIStyleAssetImporter)
public:
    bool canImport(SString fileExtension) override { return fileExtension == "rcss"; }
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override;
};

class MUIWidgetAssetImporter : public MAssetImporter
{
    DEFINE_OBJECT_SUBCLASS(MUIWidgetAssetImporter)
public:
    bool canImport(SString fileExtension) override { return fileExtension == "muia"; }
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override;
};

#endif // UI_ASSET_IMPORTERS_H
