//
// Created by ssj5v on 27-09-2024.
//

#include "textasset.h"

#include "core/utils/fileio.h"

MTextAsset::MTextAsset(SString path) : MAsset(path){
    name = "TextAsset";
}

SString MTextAsset::getText() const {
    return text;
}

bool MTextAsset::loadFromPath(const SString& path) {
    valid = FileIO::readFile(path, text);
    return valid;
}

