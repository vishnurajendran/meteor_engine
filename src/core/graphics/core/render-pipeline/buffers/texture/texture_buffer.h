//
// Texture buffer - a single colour texture, no FBO of its own.
//

#pragma once
#ifndef TEXTURE_BUFFER_H
#define TEXTURE_BUFFER_H

#include "core/graphics/core/render-pipeline/buffer.h"
#include "core/utils/glmhelper.h"

// A screen-sized colour texture registered in the buffer registry so any
// stage can find it by name. It owns no framebuffer: the stage that writes
// it attaches it to its own FBO (MPrepassStage attaches BUFFER_NORMAL and
// BUFFER_SURFACE as colour targets 0 and 1), and readers sample it.
//
// The texture handle changes on every resize, so writers must re-check
// getTextureHandle() before attaching.
//
// No GL headers here - only in the .cpp.
class STextureBuffer : public SBuffer
{
public:
    ~STextureBuffer() override;

    // No FBO - binding is done by the stage that attaches this texture.
    void bindAsActive() override {}
    void unbind()       override {}

    bool makeBuffer(const SString& bufferName) override;
    bool resize(int width, int height)         override;

    // Call before setRenderBuffer(). Default: 0x881A (GL_RGBA16F).
    // Common choices: 0x8058 GL_RGBA8, 0x822E GL_R32F.
    void setFormat(unsigned int glInternalFormat) { internalFormat = glInternalFormat; }

    [[nodiscard]] unsigned int getTextureHandle() const { return textureHandle; }
    [[nodiscard]] unsigned int getFormat()        const { return internalFormat; }
    [[nodiscard]] SVector2     getResolution()    const { return resolution; }

private:
    void destroyGL();

private:
    unsigned int textureHandle  = 0;
    unsigned int internalFormat = 0x881A; // GL_RGBA16F
    SVector2     resolution     = { 0.f, 0.f };
    SString      name;
};

#endif // TEXTURE_BUFFER_H
