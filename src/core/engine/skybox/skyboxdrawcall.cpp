//
// Created by ssj5v on 29-03-2025.
//
#include "GL/glew.h"
#include "skyboxdrawcall.h"
#include "core/engine/3d/material/material.h"
#include "core/engine/camera/viewmanagement.h"
#include "cubemapasset.h"
#include "cubemaptexture.h"

const float MSkyboxDrawCall::skyboxVertices[] = {
    // positions
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

void MSkyboxDrawCall::initialise()
{
    // skybox VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

MSkyboxDrawCall::MSkyboxDrawCall(MCubemapAsset* cubemap, MShader* shader)
{
    this->cubemapTexture = cubemap;
    this->skyboxShader = shader;
    initialise();
}

void MSkyboxDrawCall::setCubemapAsset(MCubemapAsset* cubemap)
{
    this->cubemapTexture = cubemap;
}

void MSkyboxDrawCall::draw()
{
    if (skyboxShader)
        skyboxShader->bind();

    glDepthMask(GL_FALSE);

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

    SShaderPropertyValue view;

    glm::mat4 viewMat = glm::mat4(glm::mat3(renderCamera->getViewMatrix()));
    view.setMat4Val(viewMat);
    SShaderPropertyValue projection;
    projection.setMat4Val(renderCamera->getProjectionMatrix(resolution));

    if (skyboxShader && cubemapTexture)
    {
        skyboxShader->setPropertyValue("view",  view);
        skyboxShader->setPropertyValue("projection", projection);
        cubemapTexture->getTexture()->bind(0,0);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);
    }

}


int MSkyboxDrawCall::getSortinOrder()
{
    return -9999;
}
