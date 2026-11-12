//
// Depth of field - blurs what is nearer or farther than the focus distance.
//

#include <GL/glew.h>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

#include "depth_of_field_effect.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/fullscreen_quad.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_registry.h"
#include "core/graphics/core/render-pipeline/view_data.h"

REGISTER_POST_EFFECT(MDepthOfFieldEffect, "Depth of Field")

static constexpr const char* COC_SHADER        = "meteor_assets/engine_assets/shaders/internal/postprocess/dof_coc.mesl";
static constexpr const char* PREFILTER_SHADER  = "meteor_assets/engine_assets/shaders/internal/postprocess/dof_prefilter.mesl";
static constexpr const char* BOKEH_SHADER      = "meteor_assets/engine_assets/shaders/internal/postprocess/dof_bokeh.mesl";
static constexpr const char* POSTFILTER_SHADER = "meteor_assets/engine_assets/shaders/internal/postprocess/dof_postfilter.mesl";
static constexpr const char* COMBINE_SHADER    = "meteor_assets/engine_assets/shaders/internal/postprocess/dof_combine.mesl";

SPostProcessEffectSettings::SFieldMeta SDepthOfFieldSettings::getFieldMeta(const std::string& fieldName) const
{
    SFieldMeta meta;
    if (fieldName == "focusDistance")
    {
        meta.min = 0.1f; meta.max = 200.0f;
        meta.tooltip = "Distance from the camera, in world units, that is perfectly sharp.";
    }
    else if (fieldName == "focusRange")
    {
        meta.min = 0.1f; meta.max = 100.0f;
        meta.tooltip = "How far either side of the focus distance it takes to reach full blur.\nSmaller = shallower depth of field.";
    }
    else if (fieldName == "bokehRadius")
    {
        meta.min = 1.0f; meta.max = 10.0f;
        meta.tooltip = "Strongest blur, in half-resolution pixels. Bigger looks softer but costs nothing extra.";
    }
    else if (fieldName == "showFocus")
    {
        meta.tooltip = "Debug: tint what is in front of the focus band orange and behind it blue.\nUntinted areas are in focus. Use it to tune Focus Distance and Focus Range.";
    }
    return meta;
}

void MDepthOfFieldEffect::init(IRenderPipeline* pipeline)
{
    cocShader        = loadShader(COC_SHADER);
    prefilterShader  = loadShader(PREFILTER_SHADER);
    bokehShader      = loadShader(BOKEH_SHADER);
    postfilterShader = loadShader(POSTFILTER_SHADER);
    combineShader    = loadShader(COMBINE_SHADER);
    glGenFramebuffers(1, &fbo);
}

void MDepthOfFieldEffect::cleanup()
{
    destroyTargets();
    if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
}

void MDepthOfFieldEffect::destroyTargets()
{
    for (STarget* t : { &coc, &halfA, &halfB })
    {
        if (t->texture) glDeleteTextures(1, &t->texture);
        *t = STarget{};
    }
    builtWidth = builtHeight = 0;
}

static void makeTarget(int w, int h, unsigned int& texture)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void MDepthOfFieldEffect::resize(int width, int height)
{
    if (width == builtWidth && height == builtHeight && coc.texture) return;
    destroyTargets();
    if (width <= 0 || height <= 0) return;

    const int hw = std::max(1, width / 2);
    const int hh = std::max(1, height / 2);

    coc   = { 0, width, height }; makeTarget(width, height, coc.texture);
    halfA = { 0, hw, hh };        makeTarget(hw, hh, halfA.texture);
    halfB = { 0, hw, hh };        makeTarget(hw, hh, halfB.texture);
    glBindTexture(GL_TEXTURE_2D, 0);

    builtWidth  = width;
    builtHeight = height;
}

void MDepthOfFieldEffect::drawInto(const STarget& target) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);
    glViewport(0, 0, target.width, target.height);
}

bool MDepthOfFieldEffect::apply(const SPostProcessContext& ctx)
{
    if (!cocShader || !prefilterShader || !bokehShader || !postfilterShader || !combineShader) return false;
    if (!fbo || !ctx.quad || !ctx.view || !ctx.registry) return false;

    auto* depth = ctx.registry->getBuffer<SDepthBuffer>(MBufferNames::BUFFER_DEPTH);
    if (!depth || depth->getDepthTextureHandle() == 0) return false;

    const int width  = static_cast<int>(ctx.resolution.x);
    const int height = static_cast<int>(ctx.resolution.y);
    resize(width, height);   // no-op unless the size changed
    if (!coc.texture) return false;

    const float focusDistance = std::max(settings.focusDistance.get(), 0.0f);
    const float focusRange    = std::max(settings.focusRange.get(), 0.01f);
    const float bokehRadius   = std::clamp(settings.bokehRadius.get(), 1.0f, 10.0f);

    GLint l;

    // ---- 1. Circle of confusion (full res) -------------------------------------
    {
        drawInto(coc);
        const GLuint prog = bindFullscreenShader(cocShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depth->getDepthTextureHandle());
        l = glGetUniformLocation(prog, "_depthTex");      if (l >= 0) glUniform1i(l, 0);
        l = glGetUniformLocation(prog, "_invProjection"); if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(ctx.view->inverseProjection));
        l = glGetUniformLocation(prog, "_focusDistance"); if (l >= 0) glUniform1f(l, focusDistance);
        l = glGetUniformLocation(prog, "_focusRange");    if (l >= 0) glUniform1f(l, focusRange);
        l = glGetUniformLocation(prog, "_bokehRadius");   if (l >= 0) glUniform1f(l, bokehRadius);
        ctx.quad->draw();
    }

    // ---- 2. Prefilter: half res colour + CoC -----------------------------------
    {
        drawInto(halfA);
        const GLuint prog = bindFullscreenShader(prefilterShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx.sourceTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, coc.texture);
        l = glGetUniformLocation(prog, "_source");    if (l >= 0) glUniform1i(l, 0);
        l = glGetUniformLocation(prog, "_coc");       if (l >= 0) glUniform1i(l, 1);
        l = glGetUniformLocation(prog, "_texelSize"); if (l >= 0) glUniform2f(l, 1.0f / width, 1.0f / height);
        ctx.quad->draw();
        glBindTexture(GL_TEXTURE_2D, 0);   // unit 1
    }

    // ---- 3. Bokeh gather (half res) --------------------------------------------
    {
        drawInto(halfB);
        const GLuint prog = bindFullscreenShader(bokehShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, halfA.texture);
        l = glGetUniformLocation(prog, "_source");      if (l >= 0) glUniform1i(l, 0);
        l = glGetUniformLocation(prog, "_texelSize");   if (l >= 0) glUniform2f(l, 1.0f / halfA.width, 1.0f / halfA.height);
        l = glGetUniformLocation(prog, "_bokehRadius"); if (l >= 0) glUniform1f(l, bokehRadius);
        ctx.quad->draw();
    }

    // ---- 4. Postfilter (half res) -----------------------------------------------
    {
        drawInto(halfA);
        const GLuint prog = bindFullscreenShader(postfilterShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, halfB.texture);
        l = glGetUniformLocation(prog, "_source");    if (l >= 0) glUniform1i(l, 0);
        l = glGetUniformLocation(prog, "_texelSize"); if (l >= 0) glUniform2f(l, 1.0f / halfB.width, 1.0f / halfB.height);
        ctx.quad->draw();
    }

    // ---- 5. Combine into the stage's target (full res) ---------------------------
    ctx.target->bindAsActive();
    glViewport(0, 0, width, height);

    const GLuint prog = beginFullscreen(combineShader, ctx);   // scene on unit 0
    if (!prog) return false;

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, coc.texture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, halfA.texture);
    l = glGetUniformLocation(prog, "_coc"); if (l >= 0) glUniform1i(l, 1);
    l = glGetUniformLocation(prog, "_dof"); if (l >= 0) glUniform1i(l, 2);
    l = glGetUniformLocation(prog, "_showFocus");   if (l >= 0) glUniform1i(l, settings.showFocus.get() ? 1 : 0);
    l = glGetUniformLocation(prog, "_bokehRadius"); if (l >= 0) glUniform1f(l, bokehRadius);

    drawFullscreen(ctx);   // draws, then unbinds unit 0 and the program

    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    return true;
}
