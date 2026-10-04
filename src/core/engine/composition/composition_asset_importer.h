//
// composition_asset_importer.h
//
// Registers the ".comp" extension with the asset manager.
//

#ifndef COMPOSITION_ASSET_IMPORTER_H
#define COMPOSITION_ASSET_IMPORTER_H

#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"

class MCompositionAssetImporter : public MAssetImporter
{
    DEFINE_OBJECT_SUBCLASS(MCompositionAssetImporter)

private:
    static bool registered;

public:
    MCompositionAssetImporter()           = default;
    ~MCompositionAssetImporter() override = default;

    bool    canImport(SString fileExtension) override;
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;

    // No dedicated icon yet — add e.g. HIGHRES_TEX_ASSET_COMPOSITION when one exists.
    [[nodiscard]] SString getIconPath() const override { return SEngineAssetIconPaths::HIGHRES_TEX_ASSET_DEFAULT; }
};

#endif // COMPOSITION_ASSET_IMPORTER_H
