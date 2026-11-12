//
// Fog - fades distant surfaces towards a fog colour.
//

#include <GL/glew.h>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

#include "fog_effect.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/fullscreen_quad.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_registry.h"
#include "core/graphics/core/render-pipeline/stages/lighting/shadow_inputs.h"
#include "core/graphics/core/render-pipeline/view_data.h"

REGISTER_POST_EFFECT(MFogEffect, "Fog")

static constexpr const char* FOG_SHADER = "meteor_assets/engine_assets/shaders/internal/postprocess/fog.mesl";

SPostProcessEffectSettings::SFieldMeta SFogSettings::getFieldMeta(const std::string& fieldName) const
{
    SFieldMeta meta;
    if (fieldName == "mode")
    {
        meta.options = { "Linear", "Exponential", "Exponential Squared" };
        meta.tooltip = "How fog builds up with distance.\n"
                       "Linear: none at Start Distance, full at End Distance.\n"
                       "Exponential: thickens quickly, then tapers - natural haze.\n"
                       "Exponential Squared: clear near the camera, then a sharper wall.";
    }
    else if (fieldName == "color")
    {
        meta.color   = true;
        meta.tooltip = "Fog colour (linear). Usually close to the sky colour at the horizon.";
    }
    else if (fieldName == "colorIntensity")
    {
        meta.min = 0.0f; meta.max = 10.0f;
        meta.tooltip = "Brightness multiplier for the fog colour. Fog is applied before tonemapping,\n"
                       "so raise this if fog looks dull against a brightly lit scene.";
    }
    else if (fieldName == "density")
    {
        meta.min = 0.0f; meta.max = 0.5f;
        meta.tooltip = "Exponential modes only: how thick the fog is. Small values go a long way.";
    }
    else if (fieldName == "startDistance")
    {
        meta.min = 0.0f; meta.max = 500.0f;
        meta.tooltip = "Distance from the camera, in world units, before any fog appears.";
    }
    else if (fieldName == "endDistance")
    {
        meta.min = 0.1f; meta.max = 2000.0f;
        meta.tooltip = "Linear mode only: distance at which fog reaches Max Opacity.";
    }
    else if (fieldName == "maxOpacity")
    {
        meta.min = 0.0f; meta.max = 1.0f;
        meta.tooltip = "Most fog any surface can get. Below 1, distant geometry stays faintly visible.";
    }
    else if (fieldName == "skyAmount")
    {
        meta.min = 0.0f; meta.max = 1.0f;
        meta.tooltip = "How much fog covers the sky (pixels with no geometry).\n"
                       "0 leaves the skybox untouched; raise it to blend the horizon into the fog.";
    }
    else if (fieldName == "volumetric")
    {
        meta.tooltip = "Light the fog with the directional light, shadowed per sample.\n"
                       "Shadowed air stays dark, so objects cast light shafts (god rays).\n"
                       "Needs a directional light; shafts need its shadows on.";
    }
    else if (fieldName == "volumetricDensity")
    {
        meta.min = 0.0f; meta.max = 0.2f;
        meta.tooltip = "How thick the sunlit air is, independent of the fog above.\n"
                       "Higher = brighter, shorter shafts close to the camera;\n"
                       "lower = fainter shafts that reach further.";
    }
    else if (fieldName == "sunScattering")
    {
        meta.min = 0.0f; meta.max = 2.0f;
        meta.tooltip = "Brightness of the sunlight scattered by the air (god rays).";
    }
    else if (fieldName == "showVolumetric")
    {
        meta.tooltip = "Debug: show only the scattered sun light.\n"
                       "Shafts appear as dark streaks where objects block the sun.";
    }
    else if (fieldName == "anisotropy")
    {
        meta.min = 0.0f; meta.max = 0.95f;
        meta.tooltip = "How much of the light scatters forward.\n"
                       "0: fog glows evenly in every direction.\n"
                       "Higher: brighter looking toward the sun, dimmer looking away - stronger shafts.";
    }
    else if (fieldName == "steps")
    {
        meta.min = 8.0f; meta.max = 128.0f;
        meta.tooltip = "Samples along each view ray. More = cleaner shafts, slower.";
    }
    else if (fieldName == "marchDistance")
    {
        meta.min = 10.0f; meta.max = 1000.0f;
        meta.tooltip = "Farthest distance sampled for shafts (and how far into the sky).\n"
                       "Keep it near the directional light's shadow distance: past that there\n"
                       "are no shadows to sample, so samples there are wasted.";
    }
    return meta;
}

void MFogEffect::init(IRenderPipeline* pipeline)
{
    shader = loadShader(FOG_SHADER);
}

bool MFogEffect::apply(const SPostProcessContext& ctx)
{
    const float maxOpacity = std::clamp(settings.maxOpacity.get(), 0.0f, 1.0f);
    if (maxOpacity <= 0.0f && !settings.volumetric.get()) return false;   // nothing to do
    if (!shader || !ctx.view || !ctx.view->valid || !ctx.registry) return false;

    auto* depth = ctx.registry->getBuffer<SDepthBuffer>(MBufferNames::BUFFER_DEPTH);
    if (!depth || depth->getDepthTextureHandle() == 0) return false;

    const float start = std::max(settings.startDistance.get(), 0.0f);
    const float end   = std::max(settings.endDistance.get(), start + 0.01f);
    const SVector3 color = settings.color.get() * std::max(settings.colorIntensity.get(), 0.0f);

    const GLuint prog = bindFullscreenShader(shader);
    if (!prog) return false;

    // Shadow maps take units 0..UNIT_COUNT-1 (and every shadow sampler gets
    // its own unit even without shadows), so the scene and depth go above
    // them. A sampler2D on unit 0 would clash with the cascade array's
    // sampler2DArrayShadow and GL would refuse the draw.
    MShadowInputs::bind(ctx.pipeline, prog);
    constexpr int UNIT_SOURCE = MShadowInputs::UNIT_COUNT;
    constexpr int UNIT_DEPTH  = MShadowInputs::UNIT_COUNT + 1;

    glActiveTexture(GL_TEXTURE0 + UNIT_SOURCE);
    glBindTexture(GL_TEXTURE_2D, ctx.sourceTexture);
    glActiveTexture(GL_TEXTURE0 + UNIT_DEPTH);
    glBindTexture(GL_TEXTURE_2D, depth->getDepthTextureHandle());

    GLint l;
    l = glGetUniformLocation(prog, "_source");        if (l >= 0) glUniform1i(l, UNIT_SOURCE);
    l = glGetUniformLocation(prog, "_depthTex");      if (l >= 0) glUniform1i(l, UNIT_DEPTH);
    l = glGetUniformLocation(prog, "_invProjection"); if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(ctx.view->inverseProjection));
    l = glGetUniformLocation(prog, "_invView");       if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(ctx.view->inverseView));
    l = glGetUniformLocation(prog, "_cameraPos");     if (l >= 0) glUniform3fv(l, 1, glm::value_ptr(ctx.view->cameraPosition));
    l = glGetUniformLocation(prog, "_mode");          if (l >= 0) glUniform1i(l, std::clamp(settings.mode.get(), 0, 2));
    l = glGetUniformLocation(prog, "_fogColor");      if (l >= 0) glUniform3f(l, color.x, color.y, color.z);
    l = glGetUniformLocation(prog, "_density");       if (l >= 0) glUniform1f(l, std::max(settings.density.get(), 0.0f));
    l = glGetUniformLocation(prog, "_start");         if (l >= 0) glUniform1f(l, start);
    l = glGetUniformLocation(prog, "_end");           if (l >= 0) glUniform1f(l, end);
    l = glGetUniformLocation(prog, "_maxOpacity");    if (l >= 0) glUniform1f(l, maxOpacity);
    l = glGetUniformLocation(prog, "_skyAmount");     if (l >= 0) glUniform1f(l, std::clamp(settings.skyAmount.get(), 0.0f, 1.0f));

    l = glGetUniformLocation(prog, "_volumetric");    if (l >= 0) glUniform1i(l, settings.volumetric.get() ? 1 : 0);
    l = glGetUniformLocation(prog, "_showVolumetric"); if (l >= 0) glUniform1i(l, settings.showVolumetric.get() ? 1 : 0);
    l = glGetUniformLocation(prog, "_volDensity");    if (l >= 0) glUniform1f(l, std::max(settings.volumetricDensity.get(), 0.0f));
    l = glGetUniformLocation(prog, "_steps");         if (l >= 0) glUniform1i(l, std::clamp(settings.steps.get(), 1, 256));
    l = glGetUniformLocation(prog, "_marchDistance"); if (l >= 0) glUniform1f(l, std::max(settings.marchDistance.get(), 1.0f));
    l = glGetUniformLocation(prog, "_sunScattering"); if (l >= 0) glUniform1f(l, std::max(settings.sunScattering.get(), 0.0f));
    l = glGetUniformLocation(prog, "_anisotropy");    if (l >= 0) glUniform1f(l, std::clamp(settings.anisotropy.get(), 0.0f, 0.95f));

    if (ctx.quad) ctx.quad->draw();

    glActiveTexture(GL_TEXTURE0 + UNIT_DEPTH);  glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + UNIT_SOURCE); glBindTexture(GL_TEXTURE_2D, 0);
    MShadowInputs::unbind();   // leaves GL_TEXTURE0 active
    glUseProgram(0);
    return true;
}
