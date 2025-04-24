//
// Created by ssj5v on 27-03-2025.
//

#ifndef SKYBOXENTITIY_H
#define SKYBOXENTITIY_H
#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/drawable_interface.h"
#include "cubemapasset.h"
#include "skyboxdrawcall.h"


class MMaterial;
class MCubemapAsset;

class MSkyboxEntity : public MSpatialEntity, public IMeteorDrawable {
private:
    MSkyboxDrawCall* skyboxDrawCall;

public:
    MSkyboxEntity();
    void setCubemapAsset(MCubemapAsset* cubemap);
    void prepareForDraw() override;
    void raiseDrawCall() override;

    void onDrawGizmo() override;
};

#endif //SKYBOXENTITIY_H
