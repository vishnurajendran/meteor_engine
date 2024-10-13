//
// Created by ssj5v on 12-10-2024.
//

#include "textureasset.h"
#include "texture.h"

MTextureAsset::MTextureAsset(const SString &path) : MAsset(path.c_str()) {
    valid = texture.loadFromPath(path);
}

MTexture * MTextureAsset::getTexture() {
    return &texture;
}
