//
// Created by ssj5v on 27-09-2024.
//

#include "texture.h"

#include <GL/glew.h>

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


SVector2 MTexture::getSize() const { return {coreTexture.getSize().x, coreTexture.getSize().y}; }

void MTexture::bind(const unsigned int& location, const unsigned int& index)
{
    auto textureId = getTextureID();
    glActiveTexture(GL_TEXTURE0+index);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

sf::Texture *MTexture::getCoreTexture() {
    return &coreTexture;
}

unsigned int MTexture::getTextureID() {
    return coreTexture.getNativeHandle();
}
