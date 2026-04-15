//
// Created by ssj5v on 29-03-2025.
//
#include "GL/glew.h"
#include "skyboxdrawcall.h"
#include "../../../../../engine/camera/viewmanagement.h"
#include "../../../../../engine/skybox/cubemapasset.h"
#include "../../../../../engine/skybox/cubemaptexture.h"
#include "../../render_pipeline.h"
#include "core/graphics/core/material/material.h"

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
    if (skyboxVBO != 0) glDeleteBuffers(1, &skyboxVBO);
    if (skyboxVAO != 0) glDeleteVertexArrays(1, &skyboxVAO);

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    initialised = true;
}

MSkyboxDrawCall::MSkyboxDrawCall(MCubemapAsset* cubemap, MShader* shader)
{
    this->cubemapTexture = cubemap;
    this->skyboxShader = shader;
}

void MSkyboxDrawCall::setCubemapAsset(MCubemapAsset* cubemap)
{
    this->cubemapTexture = cubemap;
}

void MSkyboxDrawCall::draw()
{
    if (!initialised)
        initialise();

    if (!skyboxShader || !cubemapTexture || skyboxVAO == 0)
    {
        MERROR("MSkyboxDrawCall::draw: Missing shader, cubemap texture, or VAO");
        return;
    }
    auto& cameras = MViewManagement::getCameras();
    if (cameras.empty())
    {
        MWARN("MSkyboxDrawCall::draw: No cameras specified");
        glDepthMask(GL_TRUE);
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

    if (!renderCamera)
    {
        MERROR("MSkyboxDrawCall::draw: Camera reference null");
        glDepthMask(GL_TRUE);
        return;
    }

    skyboxShader->bind();

    SShaderPropertyValue view;
    glm::mat4 viewMat = glm::mat4(glm::mat3(renderCamera->getViewMatrix()));
    view.setMat4Val(viewMat);

    SShaderPropertyValue projection;
    projection.setMat4Val(renderCamera->getProjectionMatrix(resolution));

    skyboxShader->setPropertyValue("view", view);
    skyboxShader->setPropertyValue("projection", projection);

    cubemapTexture->getTexture()->bind(0, 0);

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}


int MSkyboxDrawCall::getSortinOrder()
{
    return 9999;
}
