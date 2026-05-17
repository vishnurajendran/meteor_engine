//
// Created by ssj5v on 17-05-2026.
//

#include "audioclip_asset.h"

MAudioClipAsset::MAudioClipAsset(const SString& path) : MAsset(path)
{
    //Todo: load path to asset
}

MAudioClipAsset::~MAudioClipAsset()
{
    //Todo: cleanup
}

bool MAudioClipAsset::requestReload()
{
    //Todo: reload call
}

Byte* MAudioClipAsset::getRawData()
{
    //Todo: return raw data;
    return nullptr;
}
