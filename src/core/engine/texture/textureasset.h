//
// Created by ssj5v on 27-09-2024.
//

#ifndef TEXTUREASSET_H
#define TEXTUREASSET_H
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/utils/glmhelper.h"
#include "SFML/Graphics/Texture.hpp"

class MTextureAsset : public MAsset {
private:
    sf::Texture coreTexture;
public:
    explicit MTextureAsset(const SString &path)
        : MAsset(path) {
    }
    SVector2 getSize() const;
    sf::Texture* getTexture();
private:
    bool loadFromPath(const SString &path);
};

#endif //TEXTUREASSET_H
