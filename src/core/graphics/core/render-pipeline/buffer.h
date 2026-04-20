//
// Created by ssj5v on 21-03-2026.
//

#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include "core/utils/sstring.h"

// Base buffer interface. No GL headers here — concrete buffers include
// glew.h only in their .cpp files to avoid gl.h/glew.h ordering conflicts.
struct SBuffer
{
public:
    virtual void bindAsActive() = 0;
    virtual void unbind()       = 0;

    // Called by the buffer registry whenever the render resolution changes.
    // Override in concrete buffers that own GPU resources (FBOs, textures).
    virtual bool resize(int width, int height) { return true; }

    // Return false to opt out of MBufferRegistery::resizeAll().
    // Used by fixed-resolution buffers like shadow maps.
    virtual bool isResizeable() const { return true; }

    virtual bool makeBuffer(const SString& bufferName) = 0;

    SBuffer()          = default;
    virtual ~SBuffer() = default;

private:
    // Raw handle kept private; concrete types manage their own GL objects.
    unsigned int bufferHandle = 0;
};

#endif // BUFFER_H