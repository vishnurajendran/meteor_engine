//
// Created by ssj5v on 03-10-2024.
//

#pragma once
#ifndef STATICMESHDRAWCALL_H
#define STATICMESHDRAWCALL_H
#include "core/graphics/core/opengldrawcall.h"
#include "core/utils/aabb.h"


class MMaterial;
class MStaticMeshAsset;

struct SStaticMeshDrawParams {
    SVector3 position;
    MStaticMeshAsset* meshAssetRefference = nullptr;
    MMaterial* materialInstance = nullptr;
    SMatrix4 modelMatrix = SMatrix4(0);
    AABB meshBounds;
};

class MStaticMeshDrawCall : public MOpenGlDrawCall {
private:
    SStaticMeshDrawParams drawParams;
public:
    MStaticMeshDrawCall();
    void setParams(const SStaticMeshDrawParams& params);
    void draw() override;
};

#endif //STATICMESHDRAWCALL_H
