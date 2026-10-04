//
// composition_asset_importer.cpp
//

#include "composition_asset_importer.h"

#include "composition_asset.h"
#include "core/utils/logger.h"

bool MCompositionAssetImporter::registered = []()
{
    addImporter(new MCompositionAssetImporter());
    return true;
}();

bool MCompositionAssetImporter::canImport(SString fileExtension)
{
    return fileExtension == MCompositionAsset::FILE_EXTENSION;
}

MAsset* MCompositionAssetImporter::importAsset(SString path, const pugi::xml_document& /*metaData*/)
{
    auto* asset = new MCompositionAsset(path);
    if (!asset->isValid())
    {
        MERROR(SString("MCompositionAssetImporter:: invalid composition ") + path);
        delete asset;
        return nullptr;
    }
    return asset;
}
