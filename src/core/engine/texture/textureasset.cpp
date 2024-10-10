//
// Created by ssj5v on 27-09-2024.
//

#include "textureasset.h"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"

bool MTextureAsset::loadFromPath(const SString &path) {
    return coreTexture.loadFromFile(path);;
}

MTextureAsset::MTextureAsset(const SString &path) : MAsset(path) {
    valid = coreTexture.loadFromFile(path);
}

SVector2 MTextureAsset::getSize() const {
    return {coreTexture.getSize().x, coreTexture.getSize().y};
}

sf::Texture * MTextureAsset::getUITexture() {
    return &coreTexture;
}

unsigned int MTextureAsset::getTextureID() {
    return coreTexture.getNativeHandle();
}

