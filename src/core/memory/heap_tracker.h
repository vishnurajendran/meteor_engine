//
// heap_tracker.h
//
// Counts everything allocated through the global operator new / delete:
// std::vector, std::string, std::function, plain `new`, and the libraries
// linked statically into the executable (SFML, ImGui, sol2, ...).
//
// heap_tracker.cpp replaces the global operators. They still call
// malloc/free (or the aligned versions), so blocks can be freed by code that
// does not go through them, e.g. a DLL like assimp deleting an object we
// created. Sizes come from the C runtime (_msize / malloc_usable_size), not a
// header in front of the block, so mixing like that stays safe; it only makes
// the count drift by the size of those blocks.
//
// Not counted: malloc/free called directly (Lua, stb_image, miniaudio),
// the object pool's pages, Jolt (reported separately), GPU driver memory.
//

#ifndef METEOR_ENGINE_HEAP_TRACKER_H
#define METEOR_ENGINE_HEAP_TRACKER_H

#include "memory_stats.h"

namespace MHeapTracker
{
    static constexpr const char* REPORTER_NAME = "C++ heap (operator new)";

    SExternalMemoryUsage getUsage();
}

#endif // METEOR_ENGINE_HEAP_TRACKER_H
