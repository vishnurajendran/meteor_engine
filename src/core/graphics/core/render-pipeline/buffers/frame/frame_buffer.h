//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "core/graphics/core/render-pipeline/buffer.h"
#include "core/utils/glmhelper.h"

class SDepthBuffer;
class STextureBuffer;

// General-purpose offscreen FBO with:
//   - GL_RGBA8 colour texture  (sample in composite/post-process shaders)
//   - GL_DEPTH_COMPONENT24 depth renderbuffer (depth test during render),
//     or the prepass depth texture when shareDepthFrom() is used
//
// No GL headers here - only in the .cpp.
class SFrameBuffer : public SBuffer
{
public:
    ~SFrameBuffer() override;

    void bindAsActive() override;
    void unbind()       override;
    bool makeBuffer(const SString& bufferName) override;
    bool resize(int width, int height)         override;

    unsigned int getFBOHandle()          const { return fboHandle; }
    unsigned int getColorTextureHandle() const { return colorTextureHandle; }
    SVector2     getResolution()         const { return resolution; }

    // Call before setRenderBuffer() - sets the internal colour format used
    // during resize().  Default: 0x8058 (GL_RGBA8).
    // Pass 0x881A (GL_RGBA16F) for HDR buffers like BUFFER_LIGHTS.
    void setColorFormat(unsigned int glFormat) { colorFormat = glFormat; }

    // Use another buffer's depth texture (BUFFER_DEPTH, written by the prepass)
    // as this FBO's depth attachment instead of
    // allocating a private renderbuffer. Call before setRenderBuffer().
    // The attachment is (re)synced in bindAsActive(), so it survives resizes
    // regardless of the order the registry resizes buffers in.
    void shareDepthFrom(const SDepthBuffer* source) { sharedDepthSource = source; }
    [[nodiscard]] bool hasSharedDepth() const { return sharedDepthSource != nullptr; }

    // Optional second colour target (COLOR_ATTACHMENT1), e.g. BUFFER_EMISSION
    // on BUFFER_OPAQUE. Synced in bindAsActive() like the shared depth.
    // bindAsActive() always leaves only attachment 0 enabled for drawing, so
    // passes whose shaders don't write output 1 (skybox) can't leave garbage
    // in it. Call setDrawExtraColor(true) after binding to draw both.
    void setExtraColorAttachment(const STextureBuffer* source) { extraColorSource = source; }
    void setDrawExtraColor(bool enable) const;

private:
    void destroyGL();
    void syncSharedDepth();
    void syncExtraColor();

private:
    unsigned int fboHandle               = 0;
    unsigned int colorTextureHandle      = 0;
    unsigned int depthRenderbufferHandle = 0;
    unsigned int colorFormat             = 0x8058; // GL_RGBA8 by default
    const SDepthBuffer* sharedDepthSource = nullptr;
    unsigned long long attachedSharedDepth = 0;    // source storage id currently attached
    const STextureBuffer* extraColorSource = nullptr;
    unsigned long long attachedExtraColor  = 0;    // source storage id currently attached
    SVector2     resolution              = { 0.f, 0.f };
    SString      name;
};

#endif // FRAMEBUFFER_H