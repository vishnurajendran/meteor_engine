//
// Created by ssj5v on 05-10-2024.
//

#include "staticmeshentity.h"
#include "staticmeshdrawcall.h"
#include "core/engine/3d/material/material.h"
#include "core/graphics/core/graphicsrenderer.h"
#include "core/graphics/core/meteordrawables.h"

void MStaticMeshEntity::raiseDrawCall() {
    if(staticMeshAsset == nullptr) {
        MERROR("MStaticMeshEntity::raiseDrawCall(): No mesh asset");
        return;
    }

    if(materialInstance == nullptr) {
        MERROR("MStaticMeshEntity::raiseDrawCall(): No material instance");
        return;
    }

    if(drawCall == nullptr) {
        drawCall = new MStaticMeshDrawCall();
    }

    auto drawParams = SStaticMeshDrawParams();
    drawParams.materialInstance = materialInstance;
    drawParams.meshAssetRefference = staticMeshAsset;
    drawParams.modelMatrix = getModelMatrix();
    drawCall->setParams(drawParams);
    MGraphicsRenderer::submit(drawCall);
}

MStaticMeshEntity::MStaticMeshEntity() : MSpatialEntity(){
    name = "StaticMeshEntity";
    MMeteorDrawables::addToSubmitables(this);
}

MStaticMeshEntity::~MStaticMeshEntity() {
    if(materialInstance)
        delete materialInstance;
}

void MStaticMeshEntity::setStaticMeshAsset(MStaticMeshAsset *asset) {
    if(asset == nullptr)
        MERROR("MStaticMeshEntity::setStaticMeshAsset: null argument");

    this->staticMeshAsset = asset;
}

void MStaticMeshEntity::setMaterial(MMaterial *material) {
    if(material == nullptr)
        MERROR("MStaticMeshEntity::setMaterial: null argument");

    this->materialInstance = material;
}
