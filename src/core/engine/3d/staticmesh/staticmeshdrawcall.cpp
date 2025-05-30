//
// Created by ssj5v on 03-10-2024.
//

#include <GL/glew.h>
#include "staticmeshdrawcall.h"

#include "core/engine/3d/material/material.h"
#include "core/engine/3d/shader/shader.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/lighting/lighting_system_manager.h"
#include "staticmesh.h"
#include "staticmeshasset.h"

MStaticMeshDrawCall::MStaticMeshDrawCall() {
    drawParams = SStaticMeshDrawParams();
}

void MStaticMeshDrawCall::setParams(const SStaticMeshDrawParams& params) {
    drawParams = params;
}

void MStaticMeshDrawCall::draw() {
    if(!drawParams.materialInstance || !drawParams.meshAssetRefference) {
        MERROR("MStaticMeshDrawCall::draw: No mesh or material specified");
        return;
    }

    auto& cameras = MViewManagement::getCameras();
    if(cameras.empty()) {
        MWARN("MStaticMeshDrawCall::draw: No cameras specified");
        return;
    }

    MCameraEntity* renderCamera = nullptr;
    for (const auto& c : cameras)
    {
        if (c != nullptr && c->getEnabled())
        {
            renderCamera = c;
            break;
        }
    }

    if(!renderCamera) {
        MERROR("MStaticMeshDrawCall::draw: Camera Reference Null");
        return;
    }

    if(!renderCamera->getEnabled()) {
        MLOG("MStaticMeshDrawCall::draw: Camera is not enabled");
        return;
    }

    //Inform Light Manager to prepare lights for this object.
    //As I write this I realise this is kinda dumb way to do it, honestly it should
    //be query-ed and passed here, but YOLO!! I'm making this then refactoring it.
    MLightSystemManager::getInstance()->prepareDynamicLights(drawParams.meshBounds);

    SShaderPropertyValue view;
    view.setMat4Val(renderCamera->getViewMatrix());
    SShaderPropertyValue projection;
    projection.setMat4Val(renderCamera->getProjectionMatrix(resolution));

    drawParams.materialInstance->setProperty("view",  view);
    drawParams.materialInstance->setProperty("projection", projection);
    for(const auto& mesh : drawParams.meshAssetRefference->getMeshes()) {
        if(!mesh) {
            MERROR("MStaticMeshDrawCall::Mesh reference not found");
            continue;
        }
        SShaderPropertyValue model;
        model.setMat4Val(drawParams.modelMatrix);
        drawParams.materialInstance->setProperty("model", model);
        drawParams.materialInstance->bindMaterial();
        mesh->draw();
        printOpenGLDrawErrors();
    }
}
