// headless_render_buffer.h
//
// Lightweight render buffer for offscreen pipelines (thumbnails, reflection
// probes, etc.) that have no sf::RenderTarget.  Provides a fixed resolution
// and no-ops for bind/unbind -- individual stages render into their own FBOs
// so the render buffer is only needed for resolution queries and as the
// non-null guard that the pipeline checks each frame.

#pragma once
#ifndef HEADLESS_RENDER_BUFFER_H
#define HEADLESS_RENDER_BUFFER_H

#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"

class SHeadlessRenderBuffer : public SRenderBuffer
{
public:
    SHeadlessRenderBuffer(int width, int height)
        : resolution(static_cast<float>(width), static_cast<float>(height)) {}

    void bindAsActive() override {}
    void unbind()       override {}

    SVector2 getResolution() const override { return resolution; }

private:
    SVector2 resolution;
};

#endif // HEADLESS_RENDER_BUFFER_H