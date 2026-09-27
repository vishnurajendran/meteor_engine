//
// Created by ssj5v on 27-09-2024.
//

#include "textasset.h"

#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/utils/logger.h"

MTextAsset::MTextAsset(const SString& path) : MAsset(path){
    name = "TextAsset";
    valid = loadFromSource();
}

SString MTextAsset::getText() const {
    return text;
}

void MTextAsset::setText(const SString& newText) {
    text = newText;
}

bool MTextAsset::save() {
    auto target = MAssetSources::getWritable();
    if (!target)
    {
        MERROR("MTextAsset::save - active asset source is read-only: " + path);
        return false;
    }

    if (!target->writeText(path, text))
    {
        MERROR("MTextAsset::save - failed to write " + path);
        return false;
    }
    MLOG("MTextAsset::save - saved " + path);
    return true;
}

bool MTextAsset::loadFromSource() {
    return MAssetSources::getActive()->readText(path, text);
}
