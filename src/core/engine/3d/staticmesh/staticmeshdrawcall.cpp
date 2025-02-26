//
// Created by ssj5v on 03-10-2024.
//

#include <GL/glew.h>
#include "staticmeshdrawcall.h"

#include "staticmesh.h"
#include "staticmeshasset.h"
#include "core/engine/3d/material/material.h"
#include "core/engine/3d/shader/shader.h"
#include "core/engine/camera/viewmanagement.h"

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

    for(const auto& camera : cameras) {

        if(!camera) {
            MERROR("MStaticMeshDrawCall::draw: Camera Reference Null");
            continue;
        }

        if(!camera->getEnabled()) {
            MLOG("MStaticMeshDrawCall::draw: Camera is not enabled");
            continue;
        }

        SShaderPropertyValue view;
        view.setMat4Val(camera->getViewMatrix());
        SShaderPropertyValue projection;
        projection.setMat4Val(camera->getProjectionMatrix(resolution));

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
}
