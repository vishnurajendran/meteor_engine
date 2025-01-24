//
// Created by ssj5v on 22-01-2025.
//

#ifndef ENTITYFLAGS_H
#define ENTITYFLAGS_H

enum class EEntityFlags : uint32_t
{
    Default = 0,
    HideInEditor = 1 << 0,
};

// Helper functions for bitwise operations
inline EEntityFlags operator|(EEntityFlags a, EEntityFlags b) {
    return static_cast<EEntityFlags>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline EEntityFlags operator&(EEntityFlags a, EEntityFlags b) {
    return static_cast<EEntityFlags>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline EEntityFlags operator~(EEntityFlags a) {
    return static_cast<EEntityFlags>(
        ~static_cast<uint8_t>(a));
}

#endif //ENTITYFLAGS_H
