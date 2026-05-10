//
// Created by ssj5v on 29-03-2025.
//

#ifndef CUBEMAPASSET_H
#define CUBEMAPASSET_H
#include <vector>
#include "core/engine/texture/textureasset.h"

class MCubemapTexture;

class MCubemapAsset : public MTextureAsset {
    DEFINE_OBJECT_SUBCLASS(MCubemapAsset);
public:
    static constexpr int FACE_COUNT = 6;
    static const char* const FACE_LABELS[FACE_COUNT]; // "right","left","top","bottom","back","front"

    explicit MCubemapAsset(const SString& path);

    MTexture* getTexture() override;

    // Face textures (MTextureAsset) may not be in the asset manager yet when
    // the cubemap is first imported.  Deferring the GPU cubemap build until
    // after all independent assets are loaded guarantees the faces are
    // available.
    bool hasDeferredLoad() const override { return true; }
    void deferredAssetLoad(bool forced) override;

    SString getFacePath(int index)  const;
    void    setFacePath(int index, const SString& facePath);

    bool save();              // write cubemap XML with current face paths
    bool requestReload() override;  // rebuild GPU cubemap from current face paths

private:
    MCubemapTexture*     texture = nullptr;
    std::vector<SString> facePaths;  // always size 6

    bool buildCubemap();  // shared between deferredAssetLoad and requestReload
};

#endif //CUBEMAPASSET_H