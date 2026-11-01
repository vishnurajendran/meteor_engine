//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef BUFFER_NAMES_H
#define BUFFER_NAMES_H

#include "core/utils/sstring.h"

// Canonical names for pipeline-owned buffers.
// Use these everywhere instead of raw string literals.
struct MBufferNames
{
    static const SString BUFFER_DEPTH;        // prepass depth texture          (SDepthBuffer)
    static const SString BUFFER_NORMAL;       // prepass view-space normals     (STextureBuffer, RGBA16F)
    static const SString BUFFER_SURFACE;      // prepass metallic/smoothness/lit (STextureBuffer, RGBA8)
    static const SString BUFFER_OPAQUE;
    static const SString BUFFER_LIGHTS;
    static const SString BUFFER_SHADOW;
    static const SString BUFFER_TRANSPARENT;  // unused (transparents blend into BUFFER_SCENE)
    static const SString BUFFER_SCENE;        // lit linear HDR image: resolve + transparents (SFrameBuffer, RGBA16F)
    static const SString BUFFER_COMPOSITE;
};

#endif // BUFFER_NAMES_H