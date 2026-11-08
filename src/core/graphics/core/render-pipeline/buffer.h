//
// Created by ssj5v on 21-03-2026.
//

#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include "core/utils/sstring.h"

// Base buffer interface. No GL headers here - concrete buffers include
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

    // Changes every time the buffer recreates its GPU storage (resize).
    // Unique across all buffers and never reused, unlike GL texture names,
    // which drivers often hand straight back after a delete. Anything that
    // attaches another buffer's texture to its own FBO must compare this,
    // not the GL name - otherwise it keeps drawing into the deleted (but
    // still attached, so still alive) old texture.
    [[nodiscard]] unsigned long long getStorageId() const { return storageId; }

protected:
    // Call after (re)creating GPU storage.
    void newStorageId() { storageId = ++nextStorageId; }

private:
    // Raw handle kept private; concrete types manage their own GL objects.
    unsigned int bufferHandle = 0;

    unsigned long long storageId = 0;   // 0 = no storage yet
    static inline unsigned long long nextStorageId = 0;
};

#endif // BUFFER_H