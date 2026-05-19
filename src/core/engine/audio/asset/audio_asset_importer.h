//
// Created by ssj5v on 18-05-2026.
//

#ifndef AUDIO_ASSET_IMPORTER_H
#define AUDIO_ASSET_IMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"

class MAudioAssetImporter : MAssetImporter {
public:
    MAudioAssetImporter() = default;
    ~MAudioAssetImporter() override = default;

    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return SEditorAssetPaths::HIGHRES_TEX_ASSET_AUDIOCLIP; }
private:
    static bool registeredAssetImporter;
};

#endif //AUDIO_ASSET_IMPORTER_H
