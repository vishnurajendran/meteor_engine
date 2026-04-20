//
// Created by ssj5v on 21-03-2026.
//

#include "buffer_registry.h"
#include "core/utils/logger.h"

MBufferRegistery::~MBufferRegistery()
{
    for (auto& [name, buf] : mBuffers)
    {
        buf->unbind();
        delete buf;
    }
    mBuffers.clear();
    // renderBufferHandle is owned externally — do not delete.
}

void MBufferRegistery::resizeAll(int width, int height)
{
    for (auto& [name, buf] : mBuffers)
    {
        if (!buf->isResizeable()) continue;
        if (!buf->resize(width, height))
            MWARN("MBufferRegistery::resizeAll — buffer '" + name + "' resize failed");
    }
}

void MBufferRegistery::setRenderBuffer(SRenderBuffer* renderBuffer)
{
    if (renderBuffer == nullptr) return;

    if (renderBufferHandle != nullptr)
        renderBufferHandle->unbind();

    renderBufferHandle = renderBuffer;
}