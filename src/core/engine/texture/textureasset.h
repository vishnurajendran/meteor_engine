//
// Created by ssj5v on 12-10-2024.
//

#ifndef TEXTUREASSET_H
#define TEXTUREASSET_H
#include "texture.h"
#include "core/engine/assetmanagement/asset/asset.h"

class MTextureAsset : public MAsset {
private:
    MTexture texture;
public:
    MTextureAsset();
    explicit MTextureAsset(const SString& path);
    virtual MTexture* getTexture();

    bool requestReload() override { valid = texture.loadFromPath(path); return valid; };
};



#endif //TEXTUREASSET_H
