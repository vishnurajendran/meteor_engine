//
// Created by ssj5v on 27-09-2024.
//
#include "assetimporter.h"

#include "default_engine_icon_paths.h"

std::vector<MAssetImporter*>* MAssetImporter::importers;

void MAssetImporter::addImporter(MAssetImporter *importer) {
    if(importers == nullptr) {
        importers = new std::vector<MAssetImporter*>();
    }

    if(std::find(importers->begin(), importers->end(), importer) == importers->end()) {
        importers->push_back(importer);
    }
}

std::vector<MAssetImporter*>* MAssetImporter::getImporters() { return importers; }

SString MAssetImporter::getIconPath() const
{
    return SEditorAssetPaths::HIGHRES_TEX_ASSET_DEFAULT;
}
