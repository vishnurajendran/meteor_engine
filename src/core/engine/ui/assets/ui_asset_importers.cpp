//
// ui_asset_importers.cpp
//

#include "ui_asset_importers.h"

#include "core/memory/mnew.h"
#include "default_engine_icon_paths.h"
#include "ui_structure_asset.h"
#include "ui_style_asset.h"
#include "ui_widget_asset.h"

namespace
{
    template <typename TAsset>
    MAsset* importValid(const SString& path)
    {
        auto* asset = mnew<TAsset>(path);
        if (!asset->isValid())
        {
            delete asset;
            return nullptr;
        }
        return asset;
    }

    const bool registered = []()
    {
        MAssetImporter::addImporter(mnew<MUIStructureAssetImporter>());
        MAssetImporter::addImporter(mnew<MUIStyleAssetImporter>());
        MAssetImporter::addImporter(mnew<MUIWidgetAssetImporter>());
        return true;
    }();
}

MAsset* MUIStructureAssetImporter::importAsset(SString path, const pugi::xml_document&)
{
    return importValid<MUIStructureAsset>(path);
}

SString MUIStructureAssetImporter::getIconPath() const
{
    return SEngineAssetIconPaths::HIGHRES_TEX_ASSET_UI_STRUCTURE;
}

MAsset* MUIStyleAssetImporter::importAsset(SString path, const pugi::xml_document&)
{
    return importValid<MUIStyleAsset>(path);
}

SString MUIStyleAssetImporter::getIconPath() const
{
    return SEngineAssetIconPaths::HIGHRES_TEX_ASSET_UI_STYLE;
}

MAsset* MUIWidgetAssetImporter::importAsset(SString path, const pugi::xml_document&)
{
    return importValid<MUIWidgetAsset>(path);
}

SString MUIWidgetAssetImporter::getIconPath() const
{
    return SEngineAssetIconPaths::HIGHRES_TEX_ASSET_UI_WIDGET;
}
