//
// Shadow inputs - binds this frame's shadow maps for any lit shader.
//

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "shadow_inputs.h"

#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/shadow/shadow_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"

namespace MShadowInputs
{

void bind(IRenderPipeline* pipeline, unsigned int program)
{
    const GLuint prog = program;

    auto* sb = pipeline->getBufferRegistry().getBuffer<SShadowBuffer>(MBufferNames::BUFFER_SHADOW);
    const bool hasShadow = sb && sb->getFBOHandle() != 0 &&
                           (pipeline->getCompositeFlags() & ECF_Shadow);

    bool dirShadowEnabled = false;
    bool dirSmoothShadow  = false;
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
    {
        dirShadowEnabled = renderer->getLightSystem().isDirectionalShadowEnabled();
        dirSmoothShadow  = renderer->getLightSystem().isDirectionalSmoothShadow();
    }

    // ---- Directional ---------------------------------------------------------
    if (hasShadow)
    {
        glActiveTexture(GL_TEXTURE0 + UNIT_DIRECTIONAL);
        glBindTexture(GL_TEXTURE_2D, sb->getDepthTextureHandle());
    }
    {
        GLint l;
        l = glGetUniformLocation(prog, "shadowMap");
        if (l >= 0) glUniform1i(l, UNIT_DIRECTIONAL);
        l = glGetUniformLocation(prog, "lightSpaceMatrix");
        if (l >= 0 && hasShadow)
            glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(sb->lightSpaceMatrix));
        l = glGetUniformLocation(prog, "hasShadow");
        if (l >= 0) glUniform1i(l, hasShadow ? 1 : 0);
        l = glGetUniformLocation(prog, "hasDirShadow");
        if (l >= 0) glUniform1i(l, (hasShadow && dirShadowEnabled) ? 1 : 0);
        l = glGetUniformLocation(prog, "smoothShadows");
        if (l >= 0) glUniform1i(l, dirSmoothShadow ? 1 : 0);
    }

    // ---- Spot ----------------------------------------------------------------
    static const char* spotMapNames[]    = {"spotShadowMap0","spotShadowMap1","spotShadowMap2","spotShadowMap3"};
    static const char* spotMatrixNames[] = {"spotLightSpaceMatrix0","spotLightSpaceMatrix1","spotLightSpaceMatrix2","spotLightSpaceMatrix3"};
    static const char* spotPosNames[]    = {"spotLightPos0","spotLightPos1","spotLightPos2","spotLightPos3"};
    static const char* spotFarNames[]    = {"spotFarPlane0","spotFarPlane1","spotFarPlane2","spotFarPlane3"};

    // ---- Point ---------------------------------------------------------------
    static const char* pointMapNames[] = {"pointShadowMap0","pointShadowMap1","pointShadowMap2","pointShadowMap3"};
    static const char* pointFarNames[] = {"pointFarPlane0","pointFarPlane1","pointFarPlane2","pointFarPlane3"};

    if (!hasShadow) return;

    for (int i = 0; i < SShadowBuffer::MAX_SPOT_SHADOWS; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + UNIT_SPOT_BASE + i);
        glBindTexture(GL_TEXTURE_2D, (i < sb->numSpotShadows) ? sb->spotDepthTextures[i] : 0);

        GLint l = glGetUniformLocation(prog, spotMapNames[i]);
        if (l >= 0) glUniform1i(l, UNIT_SPOT_BASE + i);

        if (i >= sb->numSpotShadows) continue;
        l = glGetUniformLocation(prog, spotMatrixNames[i]);
        if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(sb->spotLightSpaceMatrices[i]));
        l = glGetUniformLocation(prog, spotPosNames[i]);
        if (l >= 0) glUniform3fv(l, 1, glm::value_ptr(sb->spotLightPositions[i]));
        l = glGetUniformLocation(prog, spotFarNames[i]);
        if (l >= 0) glUniform1f(l, sb->spotFarPlanes[i]);
    }

    for (int i = 0; i < SShadowBuffer::MAX_POINT_SHADOWS; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + UNIT_POINT_BASE + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, (i < sb->numPointShadows) ? sb->pointCubemapTextures[i] : 0);

        GLint l = glGetUniformLocation(prog, pointMapNames[i]);
        if (l >= 0) glUniform1i(l, UNIT_POINT_BASE + i);

        if (i >= sb->numPointShadows) continue;
        l = glGetUniformLocation(prog, pointFarNames[i]);
        if (l >= 0) glUniform1f(l, sb->pointFarPlanes[i]);
    }
    glActiveTexture(GL_TEXTURE0);
}

void unbind()
{
    for (int i = 0; i < UNIT_COUNT; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D,       0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

} // namespace MShadowInputs
