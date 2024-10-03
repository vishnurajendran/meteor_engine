//
// Created by ssj5v on 27-09-2024.
//

#include "textureasset.h"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"

bool MTextureAsset::loadFromPath(const SString &path) {
    valid = false;
    if(coreTexture.loadFromFile(path)) {
        valid = true;
    }
    return valid;
}

SVector2 MTextureAsset::getSize() const {
    return {coreTexture.getSize().x, coreTexture.getSize().y};
}

sf::Texture *MTextureAsset::getTexture() {
    return &coreTexture;
}
