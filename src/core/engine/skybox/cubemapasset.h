//
// Created by ssj5v on 29-03-2025.
//

#ifndef CUBEMAPASSET_H
#define CUBEMAPASSET_H
#include "core/engine/texture/textureasset.h"


class MCubemapTexture;
class MCubemapAsset : public MTextureAsset {
private:
    MCubemapTexture *texture;
public:
    MCubemapAsset(const SString& path);
    MTexture* getTexture() override;
};



#endif //CUBEMAPASSET_H
