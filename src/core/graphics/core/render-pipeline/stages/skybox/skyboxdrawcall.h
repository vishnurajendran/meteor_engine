//
// Created by ssj5v on 29-03-2025.
//

#ifndef SKYBOXDRAWCALL_H
#define SKYBOXDRAWCALL_H
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/render-pipeline/draw_calls/opengldrawcall.h"


class MCubemapAsset;
class MMaterial;
class MCubemapTexture;
class MSkyboxDrawCall : public MOpenGlDrawCall {
    DEFINE_OBJECT_SUBCLASS(MSkyboxDrawCall)
private:
    unsigned int skyboxVAO, skyboxVBO;
    MCubemapAsset* cubemapTexture;
    static const float skyboxVertices[];
    MShader* skyboxShader;
    bool initialised = false;
private:
    void initialise();

public:
    MSkyboxDrawCall(MCubemapAsset* texture, MShader* shader);
    void setCubemapAsset(MCubemapAsset* texture);
    MCubemapAsset* getCubemapAsset() const { return cubemapTexture; }
    void draw() override;
    int getSortinOrder() override;
};



#endif //SKYBOXDRAWCALL_H
