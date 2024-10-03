//
// Created by ssj5v on 27-09-2024.
//

#include "textasset.h"

#include "core/utils/fileio.h"

MTextAsset::MTextAsset(const SString& path) : MAsset(path){
    name = "TextAsset";
    valid = loadFromPath(path);
}

SString MTextAsset::getText() const {
    return text;
}

bool MTextAsset::loadFromPath(const SString& path) {
    return FileIO::readFile(path, text);
}

