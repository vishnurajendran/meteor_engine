//
// Bloom - bright parts of the HDR image bleed light into their surroundings.
//

#include <GL/glew.h>
#include <algorithm>

#include "bloom_effect.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/fullscreen_quad.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_registry.h"

REGISTER_POST_EFFECT(MBloomEffect, "Bloom")

static constexpr const char* DOWNSAMPLE_SHADER = "meteor_assets/engine_assets/shaders/internal/postprocess/bloom_downsample.mesl";
static constexpr const char* UPSAMPLE_SHADER   = "meteor_assets/engine_assets/shaders/internal/postprocess/bloom_upsample.mesl";
static constexpr const char* COMPOSITE_SHADER  = "meteor_assets/engine_assets/shaders/internal/postprocess/bloom_composite.mesl";

SPostProcessEffectSettings::SFieldMeta SBloomSettings::getFieldMeta(const std::string& fieldName) const
{
    SFieldMeta meta;
    if (fieldName == "intensity")
    {
        meta.min = 0.0f; meta.max = 3.0f;
        meta.tooltip = "How strongly the glow is added to the image.";
    }
    else if (fieldName == "threshold")
    {
        meta.min = 0.0f; meta.max = 10.0f;
        meta.tooltip = "Brightness where bloom starts, in linear units (1 = white).\nAbove 1 only HDR values bloom: bright lights and emission with intensity > 1.";
    }
    else if (fieldName == "softKnee")
    {
        meta.min = 0.0f; meta.max = 1.0f;
        meta.tooltip = "0 = hard cut at the threshold, 1 = bloom fades in gradually below it.";
    }
    else if (fieldName == "scatter")
    {
        meta.min = 0.0f; meta.max = 1.0f;
        meta.tooltip = "How far the glow spreads from bright areas.";
    }
    return meta;
}

void MBloomEffect::init(IRenderPipeline* pipeline)
{
    downsampleShader = loadShader(DOWNSAMPLE_SHADER);
    upsampleShader   = loadShader(UPSAMPLE_SHADER);
    compositeShader  = loadShader(COMPOSITE_SHADER);
    glGenFramebuffers(1, &fbo);
}

void MBloomEffect::cleanup()
{
    destroyMips();
    if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
}

void MBloomEffect::destroyMips()
{
    for (auto& m : mips)
        if (m.texture) glDeleteTextures(1, &m.texture);
    mips.clear();
    builtWidth = builtHeight = 0;
}

void MBloomEffect::resize(int width, int height)
{
    if (width == builtWidth && height == builtHeight && !mips.empty()) return;
    destroyMips();
    if (width <= 0 || height <= 0) return;

    // Half resolution first, then halve until a side drops below 8 px.
    int w = std::max(1, width  / 2);
    int h = std::max(1, height / 2);
    while (static_cast<int>(mips.size()) < MAX_MIPS && w >= 8 && h >= 8)
    {
        SMip mip;
        mip.width  = w;
        mip.height = h;
        glGenTextures(1, &mip.texture);
        glBindTexture(GL_TEXTURE_2D, mip.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        // Linear filtering is part of the filter kernels (taps sit between texels).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        mips.push_back(mip);

        w = std::max(1, w / 2);
        h = std::max(1, h / 2);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    builtWidth  = width;
    builtHeight = height;
}

void MBloomEffect::drawInto(const SMip& mip) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mip.texture, 0);
    glViewport(0, 0, mip.width, mip.height);
}

bool MBloomEffect::apply(const SPostProcessContext& ctx)
{
    if (settings.intensity.get() <= 0.0f) return false;
    if (!downsampleShader || !upsampleShader || !compositeShader || !fbo || !ctx.quad) return false;

    const int width  = static_cast<int>(ctx.resolution.x);
    const int height = static_cast<int>(ctx.resolution.y);
    resize(width, height);   // no-op unless the size changed
    if (mips.empty()) return false;

    const int mipCount = static_cast<int>(mips.size());

    // ---- 1 + 2. Prefilter, then downsample the chain --------------------------
    {
        const GLuint prog = bindFullscreenShader(downsampleShader);
        const GLint locSource    = glGetUniformLocation(prog, "_source");
        const GLint locTexel     = glGetUniformLocation(prog, "_texelSize");
        const GLint locPrefilter = glGetUniformLocation(prog, "_prefilter");
        const GLint locThreshold = glGetUniformLocation(prog, "_threshold");
        const GLint locKnee      = glGetUniformLocation(prog, "_knee");

        if (locSource    >= 0) glUniform1i(locSource, 0);
        if (locThreshold >= 0) glUniform1f(locThreshold, std::max(settings.threshold.get(), 0.0f));
        if (locKnee      >= 0) glUniform1f(locKnee, std::clamp(settings.softKnee.get(), 0.0f, 1.0f));

        glActiveTexture(GL_TEXTURE0);
        for (int i = 0; i < mipCount; ++i)
        {
            // Mip 0 reads the scene (prefiltered); each later mip reads the one above.
            const GLuint srcTex = (i == 0) ? ctx.sourceTexture : mips[i - 1].texture;
            const int    srcW   = (i == 0) ? width  : mips[i - 1].width;
            const int    srcH   = (i == 0) ? height : mips[i - 1].height;

            drawInto(mips[i]);
            glBindTexture(GL_TEXTURE_2D, srcTex);
            if (locTexel     >= 0) glUniform2f(locTexel, 1.0f / srcW, 1.0f / srcH);
            if (locPrefilter >= 0) glUniform1i(locPrefilter, i == 0 ? 1 : 0);
            ctx.quad->draw();
        }
    }

    // ---- 3. Upsample, adding each level onto the next larger one ---------------
    {
        const GLuint prog = bindFullscreenShader(upsampleShader);
        const GLint locSource  = glGetUniformLocation(prog, "_source");
        const GLint locTexel   = glGetUniformLocation(prog, "_texelSize");
        const GLint locScatter = glGetUniformLocation(prog, "_scatter");

        if (locSource  >= 0) glUniform1i(locSource, 0);
        if (locScatter >= 0) glUniform1f(locScatter, std::clamp(settings.scatter.get(), 0.0f, 1.0f));

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glActiveTexture(GL_TEXTURE0);
        for (int i = mipCount - 1; i > 0; --i)
        {
            drawInto(mips[i - 1]);
            glBindTexture(GL_TEXTURE_2D, mips[i].texture);
            if (locTexel >= 0) glUniform2f(locTexel, 1.0f / mips[i].width, 1.0f / mips[i].height);
            ctx.quad->draw();
        }
        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // ---- 4. Composite into the stage's target -------------------------------------
    ctx.target->bindAsActive();
    glViewport(0, 0, width, height);

    const GLuint prog = beginFullscreen(compositeShader, ctx);   // scene on unit 0
    if (!prog) return false;

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mips[0].texture);

    GLint l;
    l = glGetUniformLocation(prog, "_bloom");          if (l >= 0) glUniform1i(l, 1);
    l = glGetUniformLocation(prog, "_bloomTexelSize"); if (l >= 0) glUniform2f(l, 1.0f / mips[0].width, 1.0f / mips[0].height);
    l = glGetUniformLocation(prog, "_intensity");      if (l >= 0) glUniform1f(l, settings.intensity.get());

    drawFullscreen(ctx);   // draws, then unbinds unit 0 and the program

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    return true;
}
