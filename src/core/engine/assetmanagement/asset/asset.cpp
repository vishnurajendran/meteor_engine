//
// Created by ssj5v on 27-09-2024.
//

#include "asset.h"

MAsset::MAsset(const SString& path) {
    name = "Asset";
    this->path = path;
}

SString MAsset::getPath() const {
    return path;
}

bool MAsset::isValid() const {
    return valid;
}
