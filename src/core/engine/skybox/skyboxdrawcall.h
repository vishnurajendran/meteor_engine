//
// Created by ssj5v on 29-03-2025.
//

#ifndef SKYBOXDRAWCALL_H
#define SKYBOXDRAWCALL_H
#include "core/engine/3d/shader/shader.h"
#include "core/graphics/core/opengldrawcall.h"


class MCubemapAsset;
class MMaterial;
class MCubemapTexture;
class MSkyboxDrawCall : public MOpenGlDrawCall {
private:
    unsigned int skyboxVAO, skyboxVBO;
    MCubemapAsset* cubemapTexture;
    static const float skyboxVertices[];
    MShader* skyboxShader;

private:
    void initialise();
public:
    MSkyboxDrawCall(MCubemapAsset* texture, MShader* shader);
    void setCubemapAsset(MCubemapAsset* texture);
    void draw() override;
    int getSortinOrder() override;
};



#endif //SKYBOXDRAWCALL_H
