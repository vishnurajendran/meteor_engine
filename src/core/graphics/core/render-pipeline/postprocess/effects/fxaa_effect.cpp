//
// FXAA - fast approximate anti-aliasing, in screen space.
//

#include <GL/glew.h>
#include <algorithm>

#include "fxaa_effect.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_registry.h"

REGISTER_POST_EFFECT(MFXAAEffect, "FXAA")

SPostProcessEffectSettings::SFieldMeta SFXAASettings::getFieldMeta(const std::string& fieldName) const
{
    SFieldMeta meta;
    if (fieldName == "contrastThreshold")
    {
        meta.min = 0.0312f; meta.max = 0.0833f;
        meta.tooltip = "Minimum edge contrast to smooth. Lower catches more (and softer) edges.";
    }
    else if (fieldName == "relativeThreshold")
    {
        meta.min = 0.063f; meta.max = 0.333f;
        meta.tooltip = "Minimum contrast relative to local brightness. Lower smooths more in bright areas.";
    }
    else if (fieldName == "subpixelBlending")
    {
        meta.min = 0.0f; meta.max = 1.0f;
        meta.tooltip = "How much tiny details (thin lines, single pixels) are softened. 0 keeps them crisp.";
    }
    return meta;
}

void MFXAAEffect::init(IRenderPipeline* pipeline)
{
    shader = loadShader("meteor_assets/engine_assets/shaders/internal/postprocess/fxaa.mesl");
}

bool MFXAAEffect::apply(const SPostProcessContext& ctx)
{
    const GLuint prog = beginFullscreen(shader, ctx);
    if (!prog) return false;

    GLint l;
    l = glGetUniformLocation(prog, "_texelSize");
    if (l >= 0) glUniform2f(l, 1.0f / ctx.resolution.x, 1.0f / ctx.resolution.y);
    l = glGetUniformLocation(prog, "_contrastThreshold");
    if (l >= 0) glUniform1f(l, std::max(settings.contrastThreshold.get(), 0.0f));
    l = glGetUniformLocation(prog, "_relativeThreshold");
    if (l >= 0) glUniform1f(l, std::max(settings.relativeThreshold.get(), 0.0f));
    l = glGetUniformLocation(prog, "_subpixelBlending");
    if (l >= 0) glUniform1f(l, std::clamp(settings.subpixelBlending.get(), 0.0f, 1.0f));

    drawFullscreen(ctx);
    return true;
}
