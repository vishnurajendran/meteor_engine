//
// SSAO - screen-space ambient occlusion.
//

#include <GL/glew.h>
#include <algorithm>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ssao_effect.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/texture/texture_buffer.h"
#include "core/graphics/core/render-pipeline/fullscreen_quad.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_registry.h"
#include "core/graphics/core/render-pipeline/view_data.h"

REGISTER_POST_EFFECT(MSSAOEffect, "SSAO")

static constexpr const char* AO_SHADER    = "meteor_assets/engine_assets/shaders/internal/postprocess/ssao.mesl";
static constexpr const char* BLUR_SHADER  = "meteor_assets/engine_assets/shaders/internal/postprocess/ssao_blur.mesl";
static constexpr const char* APPLY_SHADER = "meteor_assets/engine_assets/shaders/internal/postprocess/ssao_apply.mesl";

SPostProcessEffectSettings::SFieldMeta SSSAOSettings::getFieldMeta(const std::string& fieldName) const
{
    SFieldMeta meta;
    if (fieldName == "radius")
    {
        meta.min = 0.05f; meta.max = 5.0f;
        meta.tooltip = "How far around each point to look for occluders, in world units.\nScale with your scene: bigger finds larger creases but gets blotchy.";
    }
    else if (fieldName == "intensity")
    {
        meta.min = 0.0f; meta.max = 4.0f;
        meta.tooltip = "How dark occluded areas get.";
    }
    else if (fieldName == "bias")
    {
        meta.min = 0.0f; meta.max = 0.2f;
        meta.tooltip = "Depth tolerance. Raise it if flat surfaces show dark speckles or banding.";
    }
    else if (fieldName == "showAO")
    {
        meta.tooltip = "Debug: show only the occlusion (white = open, dark = occluded).";
    }
    return meta;
}

void MSSAOEffect::init(IRenderPipeline* pipeline)
{
    aoShader    = loadShader(AO_SHADER);
    blurShader  = loadShader(BLUR_SHADER);
    applyShader = loadShader(APPLY_SHADER);
    glGenFramebuffers(1, &fbo);

    // Hemisphere kernel (z = along the normal), more samples close to the
    // centre so nearby occluders count most. Fixed seed: same every run.
    std::mt19937 rng(1337u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    kernel.clear();
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        glm::vec3 s(unit(rng) * 2.0f - 1.0f, unit(rng) * 2.0f - 1.0f, unit(rng));
        s = glm::normalize(s) * unit(rng);
        float scale = static_cast<float>(i) / SAMPLE_COUNT;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        kernel.push_back(s * scale);
    }
}

void MSSAOEffect::cleanup()
{
    destroyTargets();
    if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
}

void MSSAOEffect::destroyTargets()
{
    for (STarget* t : { &aoA, &aoB })
    {
        if (t->texture) glDeleteTextures(1, &t->texture);
        *t = STarget{};
    }
    builtWidth = builtHeight = 0;
}

void MSSAOEffect::resize(int width, int height)
{
    if (width == builtWidth && height == builtHeight && aoA.texture) return;
    destroyTargets();
    if (width <= 0 || height <= 0) return;

    for (STarget* t : { &aoA, &aoB })
    {
        t->width  = width;
        t->height = height;
        glGenTextures(1, &t->texture);
        glBindTexture(GL_TEXTURE_2D, t->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    builtWidth  = width;
    builtHeight = height;
}

void MSSAOEffect::drawInto(const STarget& target) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);
    glViewport(0, 0, target.width, target.height);
}

bool MSSAOEffect::apply(const SPostProcessContext& ctx)
{
    if (!aoShader || !blurShader || !applyShader) return false;
    if (!fbo || !ctx.quad || !ctx.view || !ctx.registry) return false;

    auto* depth  = ctx.registry->getBuffer<SDepthBuffer>  (MBufferNames::BUFFER_DEPTH);
    auto* normal = ctx.registry->getBuffer<STextureBuffer>(MBufferNames::BUFFER_NORMAL);
    if (!depth || depth->getDepthTextureHandle() == 0) return false;
    if (!normal || normal->getTextureHandle() == 0)    return false;

    const int width  = static_cast<int>(ctx.resolution.x);
    const int height = static_cast<int>(ctx.resolution.y);
    resize(width, height);
    if (!aoA.texture) return false;

    GLint l;

    // ---- 1. Occlusion ------------------------------------------------------------
    {
        drawInto(aoA);
        const GLuint prog = bindFullscreenShader(aoShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depth->getDepthTextureHandle());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal->getTextureHandle());

        l = glGetUniformLocation(prog, "_depthTex");      if (l >= 0) glUniform1i(l, 0);
        l = glGetUniformLocation(prog, "_normalTex");     if (l >= 0) glUniform1i(l, 1);
        l = glGetUniformLocation(prog, "_projection");    if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(ctx.view->projection));
        l = glGetUniformLocation(prog, "_invProjection"); if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(ctx.view->inverseProjection));
        l = glGetUniformLocation(prog, "_kernel");        if (l >= 0) glUniform3fv(l, static_cast<GLsizei>(kernel.size()), glm::value_ptr(kernel[0]));
        l = glGetUniformLocation(prog, "_radius");        if (l >= 0) glUniform1f(l, std::max(settings.radius.get(), 0.001f));
        l = glGetUniformLocation(prog, "_bias");          if (l >= 0) glUniform1f(l, std::max(settings.bias.get(), 0.0f));
        l = glGetUniformLocation(prog, "_intensity");     if (l >= 0) glUniform1f(l, std::max(settings.intensity.get(), 0.0f));
        ctx.quad->draw();

        glBindTexture(GL_TEXTURE_2D, 0);   // unit 1
        glActiveTexture(GL_TEXTURE0);
    }

    // ---- 2. Depth-aware blur: horizontal (A -> B), vertical (B -> A) -------------
    {
        const GLuint prog = bindFullscreenShader(blurShader);
        const GLint locSource = glGetUniformLocation(prog, "_source");
        const GLint locDir    = glGetUniformLocation(prog, "_direction");
        if (locSource >= 0) glUniform1i(locSource, 0);

        glActiveTexture(GL_TEXTURE0);
        drawInto(aoB);
        glBindTexture(GL_TEXTURE_2D, aoA.texture);
        if (locDir >= 0) glUniform2i(locDir, 1, 0);
        ctx.quad->draw();

        drawInto(aoA);
        glBindTexture(GL_TEXTURE_2D, aoB.texture);
        if (locDir >= 0) glUniform2i(locDir, 0, 1);
        ctx.quad->draw();
    }

    // ---- 3. Apply into the stage's target -------------------------------------------
    ctx.target->bindAsActive();
    glViewport(0, 0, width, height);

    const GLuint prog = beginFullscreen(applyShader, ctx);   // scene on unit 0
    if (!prog) return false;

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, aoA.texture);
    l = glGetUniformLocation(prog, "_ao");     if (l >= 0) glUniform1i(l, 1);
    l = glGetUniformLocation(prog, "_showAO"); if (l >= 0) glUniform1i(l, settings.showAO.get() ? 1 : 0);

    drawFullscreen(ctx);   // draws, then unbinds unit 0 and the program

    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    return true;
}
