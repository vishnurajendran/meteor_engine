//
// Created by ssj5v on 27-09-2024.
//

#include "textasset.h"

#include "core/utils/fileio.h"
#include "core/utils/logger.h"

MTextAsset::MTextAsset(const SString& path) : MAsset(path){
    name = "TextAsset";
    valid = loadFromPath(path);
}

SString MTextAsset::getText() const {
    return text;
}

void MTextAsset::setText(const SString& newText) {
    text = newText;
}

bool MTextAsset::save() {
    SString data = text;
    if (!FileIO::writeFile(path, data))
    {
        MERROR("MTextAsset::save — failed to write " + path);
        return false;
    }
    MLOG("MTextAsset::save — saved " + path);
    return true;
}

bool MTextAsset::loadFromPath(const SString& path) {
    return FileIO::readFile(path, text);
}