#include "process_memory.h"

#include <cstddef>

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
    // PROCESS_MEMORY_COUNTERS_EX2 (Windows 10 1809+). Declared here because
    // older SDK / MinGW headers do not have it; the layout is fixed by the OS.
    struct SProcessMemoryCountersEx2
    {
        DWORD   cb;
        DWORD   PageFaultCount;
        SIZE_T  PeakWorkingSetSize;
        SIZE_T  WorkingSetSize;
        SIZE_T  QuotaPeakPagedPoolUsage;
        SIZE_T  QuotaPagedPoolUsage;
        SIZE_T  QuotaPeakNonPagedPoolUsage;
        SIZE_T  QuotaNonPagedPoolUsage;
        SIZE_T  PagefileUsage;
        SIZE_T  PeakPagefileUsage;
        SIZE_T  PrivateUsage;
        SIZE_T  PrivateWorkingSetSize;
        ULONG64 SharedCommitUsage;
    };

    using GetProcessMemoryInfoFn = BOOL (WINAPI*)(HANDLE, void*, DWORD);

    // Lives in kernel32 since Windows 7, so no psapi link is needed.
    GetProcessMemoryInfoFn resolve()
    {
        static const auto fn = reinterpret_cast<GetProcessMemoryInfoFn>(
            reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "K32GetProcessMemoryInfo")));
        return fn;
    }
}

SProcessMemory MProcessMemory::query()
{
    SProcessMemory result;
    const auto fn = resolve();
    if (!fn)
        return result;

    SProcessMemoryCountersEx2 counters{};
    counters.cb = sizeof(counters);
    if (fn(GetCurrentProcess(), &counters, counters.cb))
    {
        result.privateWorkingSet = counters.PrivateWorkingSetSize;
    }
    else
    {
        // Older Windows: no private working set. Ask for the EX layout (same
        // fields up to PrivateUsage) and use the full working set instead.
        counters = {};
        counters.cb = static_cast<DWORD>(offsetof(SProcessMemoryCountersEx2, PrivateWorkingSetSize));
        if (!fn(GetCurrentProcess(), &counters, counters.cb))
            return result;
        result.privateWorkingSet = counters.WorkingSetSize;
    }

    result.valid          = true;
    result.workingSet     = counters.WorkingSetSize;
    result.peakWorkingSet = counters.PeakWorkingSetSize;
    result.privateCommit  = counters.PrivateUsage;
    return result;
}

#elif defined(__linux__)

#include <cstdio>
#include <cstring>

SProcessMemory MProcessMemory::query()
{
    SProcessMemory result;
    FILE* file = std::fopen("/proc/self/status", "r");
    if (!file)
        return result;

    char line[256];
    while (std::fgets(line, sizeof(line), file))
    {
        unsigned long long kb = 0;
        if (std::sscanf(line, "VmRSS: %llu kB", &kb) == 1)      result.workingSet        = kb * 1024;
        else if (std::sscanf(line, "VmHWM: %llu kB", &kb) == 1) result.peakWorkingSet    = kb * 1024;
        else if (std::sscanf(line, "RssAnon: %llu kB", &kb) == 1) result.privateWorkingSet = kb * 1024;
        else if (std::sscanf(line, "VmData: %llu kB", &kb) == 1) result.privateCommit     = kb * 1024;
    }
    std::fclose(file);
    result.valid = result.workingSet > 0;
    return result;
}

#else

SProcessMemory MProcessMemory::query()
{
    return {};
}

#endif
