//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include "frame_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/texture/texture_buffer.h"
#include "core/utils/logger.h"

SFrameBuffer::~SFrameBuffer()
{
    destroyGL();
}

void SFrameBuffer::bindAsActive()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    syncSharedDepth();
    syncExtraColor();
    if (extraColorSource) setDrawExtraColor(false);
}

void SFrameBuffer::setDrawExtraColor(bool enable) const
{
    if (fboHandle == 0) return;
    const GLenum both[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers((enable && attachedExtraColor != 0) ? 2 : 1, both);
}

// Attaches the extra colour texture if it changed (first bind or resize).
void SFrameBuffer::syncExtraColor()
{
    if (!extraColorSource || fboHandle == 0) return;

    const unsigned long long id = extraColorSource->getStorageId();
    if (id == attachedExtraColor) return;

    const unsigned int tex = extraColorSource->getTextureHandle();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex, 0);
    attachedExtraColor = id;

    if (tex != 0 && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        MERROR("SFrameBuffer::syncExtraColor - FBO incomplete after attaching extra colour for '" + name + "'");
}

// Attaches the prepass depth texture if it changed since the last bind
// (first bind, or either buffer was resized). Expects fboHandle to be bound.
void SFrameBuffer::syncSharedDepth()
{
    if (!sharedDepthSource || fboHandle == 0) return;

    // Compare storage ids, not GL names (see SBuffer::getStorageId).
    const unsigned long long id = sharedDepthSource->getStorageId();
    if (id == attachedSharedDepth) return;

    const unsigned int tex = sharedDepthSource->getDepthTextureHandle();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
    attachedSharedDepth = id;

    if (tex != 0 && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        MERROR("SFrameBuffer::syncSharedDepth - FBO incomplete after attaching prepass depth for '" + name + "'");
}

void SFrameBuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool SFrameBuffer::makeBuffer(const SString& bufferName)
{
    name = bufferName;
    return true; // actual allocation deferred to resize()
}

bool SFrameBuffer::resize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        MERROR("SFrameBuffer::resize - invalid dimensions");
        return false;
    }

    destroyGL();
    resolution = { static_cast<float>(width), static_cast<float>(height) };

    // ---- Colour texture (format set by setColorFormat, default GL_RGBA8) ---
    // For HDR buffers (e.g. BUFFER_LIGHTS), use GL_RGBA16F.
    const GLenum pixelType = (colorFormat == GL_RGBA16F || colorFormat == GL_RGBA32F)
                              ? GL_FLOAT : GL_UNSIGNED_BYTE;

    glGenTextures(1, &colorTextureHandle);
    glBindTexture(GL_TEXTURE_2D, colorTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(colorFormat), width, height, 0,
                 GL_RGBA, pixelType, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ---- Depth renderbuffer (DEPTH_COMPONENT24, matches SFML) ---------------
    // Skipped when depth is shared - the prepass texture is attached lazily
    // in bindAsActive(), because the prepass buffer may be resized after us.
    if (!sharedDepthSource)
    {
        glGenRenderbuffers(1, &depthRenderbufferHandle);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbufferHandle);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // ---- FBO ----------------------------------------------------------------
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTextureHandle, 0);
    if (depthRenderbufferHandle)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depthRenderbufferHandle);
    attachedSharedDepth = 0;   // new FBO - nothing shared attached yet
    attachedExtraColor  = 0;

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        MERROR("SFrameBuffer::resize - FBO incomplete for '" + name
               + "' (status 0x" + std::to_string(status) + ")");
        destroyGL();
        return false;
    }

    return true;
}

void SFrameBuffer::destroyGL()
{
    if (fboHandle)              { glDeleteFramebuffers(1,  &fboHandle);              fboHandle = 0; }
    if (colorTextureHandle)     { glDeleteTextures(1,      &colorTextureHandle);     colorTextureHandle = 0; }
    if (depthRenderbufferHandle){ glDeleteRenderbuffers(1, &depthRenderbufferHandle); depthRenderbufferHandle = 0; }
}