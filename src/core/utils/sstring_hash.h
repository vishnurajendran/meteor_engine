#pragma once
 
// std::hash<SString> is now specialised directly inside sstring.h.
// This header is kept for backwards compatibility with code that
// explicitly includes it.
 
#include "sstring.h"
 
// SStringHash is still available as a named functor for code that
// passes an explicit hasher template argument, e.g.:
//   std::unordered_map<SString, int, SStringHash> map;
// Prefer std::unordered_map<SString, int> (no third argument) for new code.
 
using SStringHash = SString::Hash;
 