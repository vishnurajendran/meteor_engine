//
// Created by ssj5v on 27-09-2024.
//

#include "texture.h"

#include "core/object/object.h"
#include "core/utils/glmhelper.h"

bool MTexture::loadFromPath(const SString &path) {
    return coreTexture.loadFromFile(path);
}

bool MTexture::loadFromStream(sf::InputStream &stream) {
    return coreTexture.loadFromStream(stream);
}

bool MTexture::loadFromMemory(const void *data, size_t size) {
    return coreTexture.loadFromMemory(data, size);
}


SVector2 MTexture::getSize() const {
    return {coreTexture.getSize().x, coreTexture.getSize().y};
}

sf::Texture *MTexture::getTexture() {
    return &coreTexture;
}

unsigned int MTexture::getTextureID() {
    return coreTexture.getNativeHandle();
}
