//
// Vignette - darkens the edges of the image.
//

#include <GL/glew.h>
#include "vignette_effect.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_registry.h"

REGISTER_POST_EFFECT(MVignetteEffect, "Vignette")

SPostProcessEffectSettings::SFieldMeta SVignetteSettings::getFieldMeta(const std::string& fieldName) const
{
    SFieldMeta meta;
    meta.min = 0.0f;
    meta.max = 1.0f;
    if (fieldName == "intensity")  meta.tooltip = "How dark the corners get.";
    if (fieldName == "smoothness") meta.tooltip = "How far from the edges the darkening reaches.";
    return meta;
}

void MVignetteEffect::init(IRenderPipeline* pipeline)
{
    shader = loadShader("meteor_assets/engine_assets/shaders/internal/postprocess/vignette.mesl");
}

bool MVignetteEffect::apply(const SPostProcessContext& ctx)
{
    if (settings.intensity.get() <= 0.0f) return false;   // nothing to do

    const GLuint prog = beginFullscreen(shader, ctx);
    if (!prog) return false;

    GLint l;
    l = glGetUniformLocation(prog, "_intensity");  if (l >= 0) glUniform1f(l, settings.intensity.get());
    l = glGetUniformLocation(prog, "_smoothness"); if (l >= 0) glUniform1f(l, settings.smoothness.get());

    drawFullscreen(ctx);
    return true;
}
