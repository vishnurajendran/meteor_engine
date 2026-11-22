//
// ui_style_asset.cpp
//

#include "ui_style_asset.h"

#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"

MUIStyleAsset::MUIStyleAsset(const SString& path) : MAsset(path)
{
    name  = FileIO::getFileName(path);
    valid = load();
}

bool MUIStyleAsset::requestReload()
{
    valid = load();
    return valid;
}

bool MUIStyleAsset::load()
{
    if (!MAssetSources::getActive()->readText(path, source))
    {
        MERROR("MUIStyleAsset: failed to read " + path);
        return false;
    }
    return true;
}
