//
// animation_helpers.h
//
// Small utilities shared by the animation code: the subsystem lookup, clip
// source names for messages, and hash combining for change signatures and
// cache keys. Stateless: every member is static.
//
#pragma once
#ifndef ANIMATION_HELPERS_H
#define ANIMATION_HELPERS_H

#include <cstdint>

#include "core/utils/sstring.h"

class MAnimationSubsystem;

class MAnimationHelpers
{
public:
    MAnimationHelpers() = delete;

    // 2^64 / golden ratio: spreads consecutive values across the hash range.
    static constexpr uint64_t HASH_GOLDEN_RATIO = 0x9E3779B97F4A7C15ull;
    // FNV-1a 64-bit offset basis: a non-zero starting value for hashCombine chains.
    static constexpr uint64_t HASH_SEED         = 0xCBF29CE484222325ull;

    // The registered subsystem, or nullptr (tools, early startup, after cleanup).
    static MAnimationSubsystem* getSubsystem();

    // "assets/anims/Dance.fbx" -> "Dance.fbx", for status and log messages.
    static SString displayName(const SString& assetPath);

    // Mixes `value` into `seed` (boost::hash_combine, 64-bit).
    static constexpr uint64_t hashCombine(uint64_t seed, uint64_t value)
    {
        return seed ^ (value + HASH_GOLDEN_RATIO + (seed << 6) + (seed >> 2));
    }
};

#endif // ANIMATION_HELPERS_H
