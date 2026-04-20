//
// MProceduralSkyboxDrawCall
//
#include <GL/glew.h>
#include "procedural_skybox_drawcall.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/graphics/core/shader/shader.h"
#include "core/utils/logger.h"

static constexpr float CUBE_VERTS[] = {
    -1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,    1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f,
};

MProceduralSkyboxDrawCall::MProceduralSkyboxDrawCall(MShader* shader)
    : shader(shader) {}

MProceduralSkyboxDrawCall::~MProceduralSkyboxDrawCall()
{
    if (cubeVAO) { glDeleteVertexArrays(1, &cubeVAO); cubeVAO = 0; }
    if (cubeVBO) { glDeleteBuffers(1, &cubeVBO);      cubeVBO = 0; }
}

void MProceduralSkyboxDrawCall::ensureCubeReady()
{
    if (cubeVAO != 0) return;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
}

void MProceduralSkyboxDrawCall::uploadUniforms(const SMatrix4& proj,
                                               const SMatrix4& rotView) const
{
    SShaderPropertyValue v;

    v.setMat4Val(proj);
    shader->setPropertyValue("projection", v);

    v.setMat4Val(rotView);
    shader->setPropertyValue("view", v);

    v.setVec3Val(sunDirection);
    shader->setPropertyValue("u_SunDirection", v);

    v.setFloatVal(sunSize);
    shader->setPropertyValue("u_SunSize", v);

    v.setFloatVal(sunSizeConvergence);
    shader->setPropertyValue("u_SunSizeConvergence", v);

    v.setFloatVal(atmosphereThickness);
    shader->setPropertyValue("u_AtmosphereThickness", v);

    v.setVec3Val(skyTint);
    shader->setPropertyValue("u_SkyTint", v);

    v.setVec3Val(groundColor);
    shader->setPropertyValue("u_GroundColor", v);

    v.setFloatVal(exposure);
    shader->setPropertyValue("u_Exposure", v);
}

void MProceduralSkyboxDrawCall::draw()
{
    if (!shader)
    {
        MERROR("MProceduralSkyboxDrawCall::draw — no shader");
        return;
    }

    const auto& cameras = MViewManagement::getCameras();
    if (cameras.empty())
    {
        MERROR("MProceduralSkyboxDrawCall::draw — no camera");
        return;
    }

    MCameraEntity* cam  = cameras.front();
    SMatrix4 proj       = cam->getProjectionMatrix(resolution);
    SMatrix4 rotView    = SMatrix4(SMatrix3(cam->getViewMatrix()));

    // ---- Depth ----------------------------------------------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    // ---- Face culling ---------------------------------------------------------
    // Camera is inside the cube. Default back-face culling would cull every
    // face visible from inside, so disable it for this draw.
    GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    // ---- Draw -----------------------------------------------------------------
    ensureCubeReady();
    shader->bind();
    uploadUniforms(proj, rotView);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // ---- Restore --------------------------------------------------------------
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    if (cullWasEnabled) glEnable(GL_CULL_FACE);
}