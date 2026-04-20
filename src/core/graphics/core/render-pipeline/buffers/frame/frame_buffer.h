//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "core/graphics/core/render-pipeline/buffer.h"
#include "core/utils/glmhelper.h"

// General-purpose offscreen FBO with:
//   - GL_RGBA8 colour texture  (sample in composite/post-process shaders)
//   - GL_DEPTH_COMPONENT24 depth renderbuffer (depth test during render)
//
// No GL headers here — only in the .cpp.
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

    // Call before setRenderBuffer() — sets the internal colour format used
    // during resize().  Default: 0x8058 (GL_RGBA8).
    // Pass 0x881A (GL_RGBA16F) for HDR buffers like BUFFER_LIGHTS.
    void setColorFormat(unsigned int glFormat) { colorFormat = glFormat; }

private:
    void destroyGL();

private:
    unsigned int fboHandle               = 0;
    unsigned int colorTextureHandle      = 0;
    unsigned int depthRenderbufferHandle = 0;
    unsigned int colorFormat             = 0x8058; // GL_RGBA8 by default
    SVector2     resolution              = { 0.f, 0.f };
    SString      name;
};

#endif // FRAMEBUFFER_H