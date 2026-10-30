//
// process_memory.h
//
// What the OS says this process is using. On Windows, privateWorkingSet is
// the number Task Manager shows in its "Memory" column.
//

#ifndef METEOR_ENGINE_PROCESS_MEMORY_H
#define METEOR_ENGINE_PROCESS_MEMORY_H

#include <cstddef>

struct SProcessMemory
{
    bool   valid             = false;
    size_t privateWorkingSet = 0; // private pages in RAM (Task Manager "Memory")
    size_t workingSet        = 0; // all pages in RAM, including shared DLL pages
    size_t peakWorkingSet    = 0;
    size_t privateCommit     = 0; // private memory committed, in RAM or paged out
};

namespace MProcessMemory
{
    SProcessMemory query();
}

#endif // METEOR_ENGINE_PROCESS_MEMORY_H
