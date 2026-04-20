//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include "frame_buffer.h"
#include "core/utils/logger.h"

SFrameBuffer::~SFrameBuffer()
{
    destroyGL();
}

void SFrameBuffer::bindAsActive()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
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
        MERROR("SFrameBuffer::resize — invalid dimensions");
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
    glGenRenderbuffers(1, &depthRenderbufferHandle);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbufferHandle);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // ---- FBO ----------------------------------------------------------------
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTextureHandle, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRenderbufferHandle);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        MERROR("SFrameBuffer::resize — FBO incomplete for '" + name
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