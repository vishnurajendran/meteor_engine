//
// jolt_memory_tracker.h
//
// Jolt keeps its own allocator (it is not routed through the engine pool).
// This installs Jolt allocation hooks that forward to the platform heap and
// count what Jolt holds, so the memory subsystem can report it:
//
//   MJoltMemoryTracker::install();          // instead of JPH::RegisterDefaultAllocator()
//   memory->registerExternalReporter("Jolt Physics", &MJoltMemoryTracker::getUsage);
//
// Each block carries a small header with its size (Jolt's Free() does not pass
// one). install() must run before Jolt allocates anything, and the hooks stay
// installed for the rest of the process so every block is freed by the code
// that allocated it.
//

#ifndef METEOR_ENGINE_JOLT_MEMORY_TRACKER_H
#define METEOR_ENGINE_JOLT_MEMORY_TRACKER_H

#include "core/memory/memory_stats.h"

namespace MJoltMemoryTracker
{
    void install();
    SExternalMemoryUsage getUsage();
}

#endif // METEOR_ENGINE_JOLT_MEMORY_TRACKER_H
