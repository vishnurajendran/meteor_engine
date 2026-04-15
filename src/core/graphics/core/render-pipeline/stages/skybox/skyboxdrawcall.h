//
// Created by ssj5v on 29-03-2025.
//

#ifndef SKYBOXDRAWCALL_H
#define SKYBOXDRAWCALL_H
#include "../../../shader/shader.h"
#include "../legacy_render_stage/opengldrawcall.h"


class MCubemapAsset;
class MMaterial;
class MCubemapTexture;
class MSkyboxDrawCall : public MOpenGlDrawCall {
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
    void draw() override;
    int getSortinOrder() override;
};



#endif //SKYBOXDRAWCALL_H
