//
// Created by ssj5v on 29-03-2025.
//

#ifndef SKYBOXDRAWCALL_H
#define SKYBOXDRAWCALL_H
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/skybox/cubemapasset.h"
#include "core/graphics/core/render-pipeline/draw_calls/opengldrawcall.h"
#include "core/graphics/core/shader/shader.h"


class MMaterial;
class MCubemapTexture;
class MSkyboxDrawCall : public MOpenGlDrawCall {
    DEFINE_OBJECT_SUBCLASS(MSkyboxDrawCall)
private:
    unsigned int skyboxVAO, skyboxVBO;
    TAssetHandle<MCubemapAsset> cubemapTexture;
    static const float skyboxVertices[];
    MShader* skyboxShader;
    bool initialised = false;
private:
    void initialise();

public:
    MSkyboxDrawCall(TAssetHandle<MCubemapAsset> texture, MShader* shader);
    void setCubemapAsset(TAssetHandle<MCubemapAsset> texture);
    TAssetHandle<MCubemapAsset> getCubemapAsset() const { return cubemapTexture; }
    void draw() override;
    int getSortinOrder() override;
};



#endif //SKYBOXDRAWCALL_H
