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
    static const SString BUFFER_DEPTH;
    static const SString BUFFER_OPAQUE;
    static const SString BUFFER_LIGHTS;
    static const SString BUFFER_SHADOW;
    static const SString BUFFER_TRANSPARENT;
    static const SString BUFFER_COMPOSITE;
};

#endif // BUFFER_NAMES_H