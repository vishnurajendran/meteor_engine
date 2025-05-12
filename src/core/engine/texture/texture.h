//
// Created by ssj5v on 27-09-2024.
//

#ifndef TEXTURE_H
#define TEXTURE_H
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/utils/glmhelper.h"
#include "SFML/Graphics/Texture.hpp"

class MTexture : public MObject {
private:
    sf::Texture coreTexture;
public:
    MTexture()= default;
    SVector2 getSize() const;
    virtual void bind(const unsigned int& location, const unsigned int& index);
    virtual sf::Texture* getCoreTexture();
    virtual unsigned int getTextureID();
    virtual bool loadFromPath(const SString &path);
    virtual bool loadFromStream(sf::InputStream &stream);
    virtual bool loadFromMemory(const void* data, size_t size);
};

#endif //TEXTUREASSET_H
