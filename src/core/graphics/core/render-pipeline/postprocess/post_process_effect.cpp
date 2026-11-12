//
// Post-process effect - the GPU half of an effect.
//

#include <GL/glew.h>

#include "post_process_effect.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/fullscreen_quad.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"

unsigned int MPostProcessEffect::bindFullscreenShader(MShader* shader)
{
    if (!shader) return 0;
    shader->bind();

    SShaderPropertyValue identity;
    identity.setMat4Val(glm::mat4(1.f));
    shader->setPropertyValue("model",      identity);
    shader->setPropertyValue("view",       identity);
    shader->setPropertyValue("projection", identity);

    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    return static_cast<unsigned int>(prog);
}

unsigned int MPostProcessEffect::beginFullscreen(MShader* shader, const SPostProcessContext& ctx)
{
    const GLuint progId = bindFullscreenShader(shader);
    if (!progId) return 0;
    const GLint prog = static_cast<GLint>(progId);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.sourceTexture);
    const GLint loc = glGetUniformLocation(static_cast<GLuint>(prog), "_source");
    if (loc >= 0) glUniform1i(loc, 0);

    return static_cast<unsigned int>(prog);
}

void MPostProcessEffect::drawFullscreen(const SPostProcessContext& ctx)
{
    if (ctx.quad) ctx.quad->draw();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

MShader* MPostProcessEffect::loadShader(const char* path)
{
    auto asset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
                     ->getAsset<MShaderAsset>(path);
    if (!asset || !asset->getShader())
    {
        MERROR("Post-process effect: could not load shader " + SString(path));
        return nullptr;
    }
    return asset->getShader();
}
