//
// Created by ssj5v on 22-01-2025.
//
#pragma once
#include "sstring.h"

// Specialize std::hash for SString
struct SStringHash {
    std::size_t operator()(const SString& s) const noexcept {
        return std::hash<std::string>{}(s.str());  // Assuming SString has a `str()` method
    }
};