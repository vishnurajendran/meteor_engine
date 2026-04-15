//
// Created by ssj5v on 21-03-2026.
//

#include <GL/glew.h>
#include "depthbuffer.h"
#include "core/utils/logger.h"

SDepthBuffer::~SDepthBuffer()
{
    destroyGL();
}

// ---------------------------------------------------------------------------
// SBuffer interface
// ---------------------------------------------------------------------------

void SDepthBuffer::bindAsActive()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
}

void SDepthBuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool SDepthBuffer::makeBuffer(const SString& bufferName)
{
    // Allocation is deferred to resize() so the caller can supply dimensions.
    name = bufferName;
    return true;
}

// ---------------------------------------------------------------------------
// Resize / allocate
// ---------------------------------------------------------------------------

bool SDepthBuffer::resize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        MERROR("SDepthBuffer::resize — invalid dimensions (" +
               std::to_string(width) + "x" + std::to_string(height) + ")");
        return false;
    }

    destroyGL();

    resolution = { static_cast<float>(width), static_cast<float>(height) };

    // ---- Depth texture -------------------------------------------------------
    glGenTextures(1, &depthTextureHandle);
    glBindTexture(GL_TEXTURE_2D, depthTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                 width, height, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ---- FBO — depth attachment only ----------------------------------------
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, depthTextureHandle, 0);

    // No colour attachment — tell the driver explicitly.
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        MERROR("SDepthBuffer::resize — FBO incomplete (status 0x" +
               std::to_string(status) + ")");
        destroyGL();
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void SDepthBuffer::destroyGL()
{
    if (fboHandle != 0)
    {
        glDeleteFramebuffers(1, &fboHandle);
        fboHandle = 0;
    }
    if (depthTextureHandle != 0)
    {
        glDeleteTextures(1, &depthTextureHandle);
        depthTextureHandle = 0;
    }
}