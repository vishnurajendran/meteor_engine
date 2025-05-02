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
};



#endif //TEXTUREASSET_H
