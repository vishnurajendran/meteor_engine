//
// Created by ssj5v on 27-09-2024.
//

#ifndef TEXTASSETIMPORTER_H
#define TEXTASSETIMPORTER_H
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "default_engine_icon_paths.h"

/// Importer for Text Assets
class MTextAssetImporter : public MAssetImporter {
    DEFINE_OBJECT_SUBCLASS(MTextAssetImporter)
private:
    static bool registered;
public:
    MTextAssetImporter() = default;
    ~MTextAssetImporter() override = default;

    bool canImport(SString fileExtension) override;
    MAsset * importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override { return SEditorAssetPaths::HIGHRES_TEX_ASSET_TEXT; }
};



#endif //TEXTASSETIMPORTER_H
