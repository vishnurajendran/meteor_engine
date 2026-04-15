//
// Created by ssj5v on 21-03-2026.
//

#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <GL/gl.h>
#include "core/utils/sstring.h"

struct SBuffer
{
public:
    virtual void bindAsActive() = 0;
    virtual void unbind()       = 0;

    // Called by the buffer registry whenever the render resolution changes.
    // Override in concrete buffers that own GPU resources (FBOs, textures).
    // Default is a no-op so simple or externally-managed buffers are unaffected.
    virtual bool resize(int width, int height) { return true; }

    virtual bool makeBuffer(const SString& bufferName) = 0;

    SBuffer()          = default;
    virtual ~SBuffer() = default;

private:
    GLuint bufferHandle = 0;
};

#endif // BUFFER_H